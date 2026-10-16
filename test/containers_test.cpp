// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Pourya Afshintabar

#include <new>
#include <type_traits>
#include <utility>

#include "heap_fixture.hpp"
#include "psram/containers.hpp"

namespace {

using ContainersTest = HeapTest;

TEST_F(ContainersTest, VectorStoresElementsInExternalRegion) {
    const psram::vector<int> v(1000, 7);
    EXPECT_EQ(v.size(), 1000u);
    EXPECT_EQ(v[999], 7);
    EXPECT_GE(external().used, 1000 * sizeof(int));
    EXPECT_EQ(internal().allocations, 0u);
}

TEST_F(ContainersTest, NodeContainersStoreEveryNodeInExternalRegion) {
    psram::map<int, int> squares;
    psram::list<int> order;
    psram::set<int> seen;
    psram::deque<int> queue;
    for (int i = 0; i < 50; ++i) {
        squares.emplace(i, i * i);
        order.push_back(i);
        seen.insert(i);
        queue.push_back(i);
    }
    EXPECT_EQ(squares.at(7), 49);
    EXPECT_EQ(order.size(), 50u);
    EXPECT_EQ(seen.count(49), 1u);
    EXPECT_EQ(queue.back(), 49);
    EXPECT_GE(external().allocations, 150u);  // at least one node per element
    EXPECT_EQ(internal().allocations, 0u);
}

TEST_F(ContainersTest, LongStringsUseExternalRegion) {
    psram::string s = "short";  // fits in the small-string buffer, no heap use
    EXPECT_EQ(external().allocations, 0u);
    s.assign(200, 'x');
    EXPECT_EQ(s.size(), 200u);
    EXPECT_EQ(external().allocations, 1u);
    EXPECT_EQ(internal().allocations, 0u);
}

TEST_F(ContainersTest, UnorderedMapAcceptsPsramStringKeys) {
    static_assert(std::is_same_v<psram::default_hash_t<psram::string>, psram::string_hash>);
    static_assert(std::is_same_v<psram::default_hash_t<int>, std::hash<int>>);

    psram::unordered_map<psram::string, int> counts;
    counts[psram::string(40, 'a')] = 1;
    counts[psram::string(40, 'b')] = 2;
    EXPECT_EQ(counts.size(), 2u);
    EXPECT_EQ(counts.at(psram::string(40, 'b')), 2);
    EXPECT_EQ(internal().allocations, 0u);
}

TEST_F(ContainersTest, StringHashMatchesStdHash) {
    const psram::string s(40, 'q');
    EXPECT_EQ(psram::string_hash{}(s), std::hash<std::string_view>{}(std::string_view(s)));
}

TEST_F(ContainersTest, NestedContainersStayInExternalRegion) {
    const psram::vector<psram::vector<int>> grid(10, psram::vector<int>(10, 1));
    EXPECT_EQ(grid[9][9], 1);
    EXPECT_EQ(external().allocations, 12u);  // ten rows, the outer array and the temporary prototype row
    EXPECT_EQ(internal().allocations, 0u);
}

TEST_F(ContainersTest, FallbackContainersUseInternalRegionWithoutPsram) {
    set_capacity(0);
    const psram::fallback::vector<int> v(10, 1);
    const psram::fallback::string s(100, 'y');
    EXPECT_EQ(v.size(), 10u);
    EXPECT_EQ(s.size(), 100u);
    EXPECT_EQ(internal().allocations, 2u);
}

TEST_F(ContainersTest, ExternalOnlyContainerThrowsWithoutPsram) {
    set_capacity(0);
    EXPECT_THROW(
        {
            psram::vector<int> v(10);
            (void)v;
        },
        std::bad_alloc);
}

TEST_F(ContainersTest, SwapAndMoveKeepTheSameStorage) {
    psram::vector<int> a(100, 1);
    psram::vector<int> b(50, 2);
    const int* const a_data = a.data();
    a.swap(b);
    EXPECT_EQ(b.data(), a_data);

    const psram::vector<int> c = std::move(b);
    EXPECT_EQ(c.data(), a_data);
    EXPECT_EQ(external().allocations, 2u);
}

}  // namespace
