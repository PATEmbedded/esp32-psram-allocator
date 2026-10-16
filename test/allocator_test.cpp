// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Pourya Afshintabar

#include <cstdint>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>

#include "heap_fixture.hpp"
#include "psram/allocator.hpp"

namespace {

struct Tracked {
    static inline int alive = 0;

    int value;

    explicit Tracked(int v) : value(v) {
        ++alive;
    }
    Tracked(const Tracked& other) : value(other.value) {
        ++alive;
    }
    Tracked& operator=(const Tracked&) = default;
    ~Tracked() {
        --alive;
    }
};

struct alignas(64) CacheLine {
    std::uint8_t bytes[64];
};

struct ThrowsOnConstruction {
    ThrowsOnConstruction() {
        throw std::runtime_error("constructor failed");
    }
};

using AllocatorTest = HeapTest;

TEST_F(AllocatorTest, AllocatesFromExternalRegion) {
    psram::allocator<int> alloc;
    int* p = alloc.allocate(100);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(external().allocations, 1u);
    EXPECT_EQ(external().used, 100 * sizeof(int));
    EXPECT_EQ(internal().allocations, 0u);
    alloc.deallocate(p, 100);
    EXPECT_EQ(external().used, 0u);
}

TEST_F(AllocatorTest, ZeroSizedRequestAllocatesNothing) {
    psram::allocator<int> alloc;
    int* p = alloc.allocate(0);
    EXPECT_EQ(p, nullptr);
    EXPECT_EQ(external().allocations, 0u);
    alloc.deallocate(p, 0);
}

TEST_F(AllocatorTest, ThrowsWhenExternalRegionIsMissing) {
    set_capacity(0);
    psram::allocator<int> alloc;
    EXPECT_THROW((void)alloc.allocate(1), std::bad_alloc);
    EXPECT_EQ(internal().allocations, 0u);
}

TEST_F(AllocatorTest, ThrowsWhenRequestIsLargerThanMaxSize) {
    psram::allocator<std::uint64_t> alloc;
    EXPECT_THROW((void)alloc.allocate(alloc.max_size() + 1), std::bad_alloc);
}

TEST_F(AllocatorTest, FallbackPolicyUsesInternalRegionWhenExternalIsFull) {
    set_capacity(16);
    psram::allocator<int, psram::prefer_external> alloc;
    int* p = alloc.allocate(64);  // 256 bytes do not fit into 16 bytes of PSRAM
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(external().allocations, 0u);
    EXPECT_EQ(internal().allocations, 1u);
    alloc.deallocate(p, 64);
}

TEST_F(AllocatorTest, FallbackPolicyPrefersExternalRegion) {
    psram::allocator<int, psram::prefer_external> alloc;
    int* p = alloc.allocate(64);
    EXPECT_EQ(external().allocations, 1u);
    EXPECT_EQ(internal().allocations, 0u);
    alloc.deallocate(p, 64);
}

TEST_F(AllocatorTest, HonoursAlignmentOfOverAlignedTypes) {
    psram::allocator<CacheLine> alloc;
    CacheLine* p = alloc.allocate(3);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(p) % alignof(CacheLine), 0u);
    EXPECT_EQ(psram::backend::heap().live.at(p).alignment, alignof(CacheLine));
    alloc.deallocate(p, 3);
}

TEST_F(AllocatorTest, InstancesCompareEqualAndRebind) {
    using IntAlloc = psram::allocator<int>;
    using Traits = std::allocator_traits<IntAlloc>;
    static_assert(Traits::is_always_equal::value);
    static_assert(std::is_same_v<Traits::rebind_alloc<char>, psram::allocator<char>>);
    static_assert(std::is_same_v<std::allocator_traits<psram::allocator<int, psram::prefer_external>>::rebind_alloc<char>,
                                 psram::allocator<char, psram::prefer_external>>);

    const IntAlloc a{};
    const psram::allocator<double> b{a};
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
}

TEST_F(AllocatorTest, MakeUniqueConstructsAndDestroysInExternalRegion) {
    Tracked::alive = 0;
    {
        const auto p = psram::make_unique<Tracked>(42);
        ASSERT_NE(p, nullptr);
        EXPECT_EQ(p->value, 42);
        EXPECT_EQ(Tracked::alive, 1);
        EXPECT_EQ(external().allocations, 1u);
    }
    EXPECT_EQ(Tracked::alive, 0);
}

TEST_F(AllocatorTest, UniquePtrHasNoSizeOverhead) {
    static_assert(sizeof(psram::unique_ptr<Tracked>) == sizeof(Tracked*));
}

TEST_F(AllocatorTest, MakeUniqueWithFallbackPolicy) {
    set_capacity(0);
    const auto p = psram::make_unique<Tracked, psram::prefer_external>(7);
    EXPECT_EQ(p->value, 7);
    EXPECT_EQ(internal().allocations, 1u);
}

TEST_F(AllocatorTest, MakeUniqueReleasesMemoryWhenConstructorThrows) {
    EXPECT_THROW((void)psram::make_unique<ThrowsOnConstruction>(), std::runtime_error);
    EXPECT_TRUE(psram::backend::heap().live.empty());
}

TEST_F(AllocatorTest, MakeUniqueThrowsWhenExternalRegionIsMissing) {
    set_capacity(0);
    EXPECT_THROW((void)psram::make_unique<Tracked>(1), std::bad_alloc);
}

TEST_F(AllocatorTest, ReportsExternalRegionUsage) {
    set_capacity(1024);
    EXPECT_TRUE(psram::available());
    EXPECT_EQ(psram::free_bytes(), 1024u);

    psram::allocator<char> alloc;
    char* p = alloc.allocate(100);
    EXPECT_EQ(psram::free_bytes(), 924u);
    EXPECT_EQ(psram::largest_free_block(), 924u);
    alloc.deallocate(p, 100);
}

TEST_F(AllocatorTest, ReportsMissingExternalRegion) {
    set_capacity(0);
    EXPECT_FALSE(psram::available());
}

}  // namespace
