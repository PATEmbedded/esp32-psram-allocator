// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Pourya Afshintabar
//
// std::pmr memory resources backed by PSRAM. They need a standard library with
// <memory_resource>: GCC 9 or newer, for example the ESP-IDF 5.x toolchains.

#pragma once

#include "allocator.hpp"

#if !__has_include(<memory_resource>)
#error "<memory_resource> is not available: it needs GCC 9 or newer (for example ESP-IDF 5.x)"
#endif

#include <memory_resource>

namespace psram {

/// std::pmr::memory_resource that allocates with the given policy.
template <class Policy>
class basic_memory_resource final : public std::pmr::memory_resource {
private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        // A zero-byte request still needs a unique, non-null pointer.
        void* block = detail::allocate_bytes<Policy>(bytes == 0 ? 1 : bytes, alignment);
        if (block == nullptr) {
            detail::allocation_failed();
        }
        return block;
    }

    void do_deallocate(void* block, std::size_t /*bytes*/, std::size_t /*alignment*/) override {
        backend::deallocate(block);
    }

    [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
};

/// Shared resource that allocates from PSRAM only.
[[nodiscard]] inline std::pmr::memory_resource* external_resource() noexcept {
    static basic_memory_resource<external_only> resource;
    return &resource;
}

/// Shared resource that allocates from PSRAM, and from internal RAM when PSRAM
/// is missing or full.
[[nodiscard]] inline std::pmr::memory_resource* fallback_resource() noexcept {
    static basic_memory_resource<prefer_external> resource;
    return &resource;
}

}  // namespace psram
