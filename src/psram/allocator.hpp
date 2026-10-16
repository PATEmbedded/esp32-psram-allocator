// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Pourya Afshintabar
//
// Standard allocators that take their memory from the external PSRAM of ESP32
// chips, and psram::make_unique() for single objects.

#pragma once

#if __cplusplus < 201703L
#error "esp32-psram-allocator needs C++17. On Arduino-ESP32 2.x replace -std=gnu++11 with -std=gnu++17."
#endif

#include <cstddef>
#include <cstdlib>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

#include "backend.hpp"

namespace psram {

/// Policy: allocate from external PSRAM only. Allocation fails when PSRAM is
/// missing or full.
struct external_only {
    static constexpr bool fallback_to_internal = false;
};

/// Policy: allocate from external PSRAM, and from internal RAM when PSRAM is
/// missing or full.
struct prefer_external {
    static constexpr bool fallback_to_internal = true;
};

namespace detail {

/// Reports an allocation failure the way the standard library does: throws
/// std::bad_alloc, or aborts when exceptions are disabled (the ESP-IDF default).
[[noreturn]] inline void allocation_failed() {
#if defined(__cpp_exceptions)
    throw std::bad_alloc();
#else
    std::abort();
#endif
}

template <class Policy>
[[nodiscard]] inline void* allocate_bytes(std::size_t bytes, std::size_t alignment) noexcept {
    void* block = backend::allocate(bytes, alignment, region::external);
    if constexpr (Policy::fallback_to_internal) {
        if (block == nullptr) {
            block = backend::allocate(bytes, alignment, region::internal);
        }
    }
    return block;
}

}  // namespace detail

/// Allocator for standard containers that places their storage in PSRAM.
///
/// It holds no state and all instances compare equal, so containers that use
/// it move, swap and copy like containers that use std::allocator.
template <class T, class Policy = external_only>
class allocator {
public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using propagate_on_container_move_assignment = std::true_type;
    using is_always_equal = std::true_type;

    template <class U>
    struct rebind {
        using other = allocator<U, Policy>;
    };

    constexpr allocator() noexcept = default;

    template <class U>
    constexpr allocator(const allocator<U, Policy>& /*other*/) noexcept {}

    /// Allocates uninitialised storage for `n` objects of type T.
    [[nodiscard]] T* allocate(std::size_t n) {
        if (n == 0) {
            return nullptr;
        }
        if (n > max_size()) {
            detail::allocation_failed();
        }
        void* block = detail::allocate_bytes<Policy>(n * sizeof(T), alignof(T));
        if (block == nullptr) {
            detail::allocation_failed();
        }
        return static_cast<T*>(block);
    }

    void deallocate(T* p, std::size_t /*n*/) noexcept {
        backend::deallocate(p);
    }

    [[nodiscard]] constexpr std::size_t max_size() const noexcept {
        return std::numeric_limits<std::size_t>::max() / sizeof(T);
    }
};

template <class T, class PolicyT, class U, class PolicyU>
constexpr bool operator==(const allocator<T, PolicyT>& /*lhs*/, const allocator<U, PolicyU>& /*rhs*/) noexcept {
    return true;
}

template <class T, class PolicyT, class U, class PolicyU>
constexpr bool operator!=(const allocator<T, PolicyT>& /*lhs*/, const allocator<U, PolicyU>& /*rhs*/) noexcept {
    return false;
}

/// Deleter of psram::unique_ptr: destroys the object and frees its memory.
template <class T>
struct deleter {
    void operator()(T* p) const noexcept {
        if (p != nullptr) {
            p->~T();
            backend::deallocate(p);
        }
    }
};

/// std::unique_ptr for objects made by psram::make_unique(). It does not
/// convert to a pointer to a base class, because the memory has to be freed at
/// the address it was allocated at.
template <class T>
using unique_ptr = std::unique_ptr<T, deleter<T>>;

/// Creates one object in PSRAM, like std::make_unique.
template <class T, class Policy = external_only, class... Args>
[[nodiscard]] unique_ptr<T> make_unique(Args&&... args) {
    static_assert(!std::is_array_v<T>, "psram::make_unique does not create arrays; use psram::vector");
    void* block = detail::allocate_bytes<Policy>(sizeof(T), alignof(T));
    if (block == nullptr) {
        detail::allocation_failed();
    }
#if defined(__cpp_exceptions)
    try {
        return unique_ptr<T>(::new (block) T(std::forward<Args>(args)...));
    } catch (...) {
        backend::deallocate(block);
        throw;
    }
#else
    return unique_ptr<T>(::new (block) T(std::forward<Args>(args)...));
#endif
}

/// True when external PSRAM is present and part of the heap.
[[nodiscard]] inline bool available() noexcept {
    return backend::total_size(region::external) > 0;
}

/// Free bytes in external PSRAM.
[[nodiscard]] inline std::size_t free_bytes() noexcept {
    return backend::free_size(region::external);
}

/// Size of the largest block external PSRAM can provide right now.
[[nodiscard]] inline std::size_t largest_free_block() noexcept {
    return backend::largest_free_block(region::external);
}

}  // namespace psram
