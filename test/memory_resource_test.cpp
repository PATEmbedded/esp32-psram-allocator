// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Pourya Afshintabar

#if __has_include(<memory_resource>)

#include <cstdint>
#include <memory_resource>
#include <new>
#include <string>
#include <vector>

#include "heap_fixture.hpp"
#include "psram/memory_resource.hpp"

namespace {

using MemoryResourceTest = HeapTest;

TEST_F(MemoryResourceTest, PmrContainersAllocateFromExternalRegion) {
    std::pmr::vector<int> values{psram::external_resource()};
    values.assign(100, 5);
    const std::pmr::string text(100, 'z', psram::external_resource());
    EXPECT_EQ(values.back(), 5);
    EXPECT_EQ(text.size(), 100u);
    EXPECT_EQ(external().allocations, 2u);
    EXPECT_EQ(internal().allocations, 0u);
}

TEST_F(MemoryResourceTest, ZeroByteRequestReturnsUsableBlock) {
    std::pmr::memory_resource* resource = psram::external_resource();
    void* block = resource->allocate(0);
    EXPECT_NE(block, nullptr);
    resource->deallocate(block, 0);
}

TEST_F(MemoryResourceTest, HonoursRequestedAlignment) {
    std::pmr::memory_resource* resource = psram::external_resource();
    void* block = resource->allocate(100, 128);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(block) % 128, 0u);
    resource->deallocate(block, 100, 128);
}

TEST_F(MemoryResourceTest, FallbackResourceUsesInternalRegionWithoutPsram) {
    set_capacity(0);
    std::pmr::vector<int> values{psram::fallback_resource()};
    values.resize(10);
    EXPECT_EQ(internal().allocations, 1u);
}

TEST_F(MemoryResourceTest, ExternalResourceThrowsWithoutPsram) {
    set_capacity(0);
    EXPECT_THROW((void)psram::external_resource()->allocate(16), std::bad_alloc);
}

TEST_F(MemoryResourceTest, ResourcesCompareByIdentity) {
    EXPECT_TRUE(psram::external_resource()->is_equal(*psram::external_resource()));
    EXPECT_FALSE(psram::external_resource()->is_equal(*psram::fallback_resource()));
}

}  // namespace

#endif  // __has_include(<memory_resource>)
