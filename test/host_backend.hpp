// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Pourya Afshintabar
//
// Host stand-in for the heap_caps backend, used by the unit tests.
//
// It simulates the two regions with a fixed capacity and records every live
// block, so the tests can check which region served a request, that the
// requested alignment was honoured and that nothing leaks.

#pragma once

#include <cstddef>
#include <cstdlib>
#include <map>
#include <new>

namespace psram::backend {

struct block_info {
    std::size_t bytes;
    std::size_t alignment;
    region where;
};

struct region_state {
    std::size_t capacity;
    std::size_t used = 0;
    std::size_t allocations = 0;
};

inline constexpr std::size_t default_external_capacity = 8u * 1024u * 1024u;  // an 8 MB PSRAM chip
inline constexpr std::size_t default_internal_capacity = 320u * 1024u;

struct host_heap {
    region_state external{default_external_capacity};
    region_state internal{default_internal_capacity};
    std::map<void*, block_info> live;

    region_state& state(region r) {
        return r == region::external ? external : internal;
    }

    /// Sets new capacities and clears the statistics. Every block of the
    /// previous test must have been released.
    void reset(std::size_t external_capacity = default_external_capacity,
               std::size_t internal_capacity = default_internal_capacity) {
        external = region_state{external_capacity};
        internal = region_state{internal_capacity};
        live.clear();
    }
};

inline host_heap& heap() {
    static host_heap instance;
    return instance;
}

namespace detail {

[[nodiscard]] constexpr std::size_t new_alignment(std::size_t requested) noexcept {
    return requested > alignof(std::max_align_t) ? requested : alignof(std::max_align_t);
}

}  // namespace detail

[[nodiscard]] inline void* allocate(std::size_t bytes, std::size_t alignment, region r) noexcept {
    region_state& state = heap().state(r);
    if (bytes > state.capacity - state.used) {
        return nullptr;
    }
    void* block = ::operator new(bytes, std::align_val_t{detail::new_alignment(alignment)}, std::nothrow);
    if (block == nullptr) {
        return nullptr;
    }
    state.used += bytes;
    ++state.allocations;
    heap().live.emplace(block, block_info{bytes, alignment, r});
    return block;
}

inline void deallocate(void* block) noexcept {
    if (block == nullptr) {
        return;
    }
    auto& live = heap().live;
    const auto it = live.find(block);
    if (it == live.end()) {
        std::abort();  // not a block from allocate(): a bug in the code under test
    }
    heap().state(it->second.where).used -= it->second.bytes;
    ::operator delete(block, std::align_val_t{detail::new_alignment(it->second.alignment)});
    live.erase(it);
}

[[nodiscard]] inline std::size_t total_size(region r) noexcept {
    return heap().state(r).capacity;
}

[[nodiscard]] inline std::size_t free_size(region r) noexcept {
    const region_state& state = heap().state(r);
    return state.capacity - state.used;
}

[[nodiscard]] inline std::size_t largest_free_block(region r) noexcept {
    return free_size(r);  // the simulated heap does not fragment
}

}  // namespace psram::backend
