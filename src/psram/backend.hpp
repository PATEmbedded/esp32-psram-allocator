// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Pourya Afshintabar
//
// Raw memory backend for the allocators in <psram/allocator.hpp>.
//
// On ESP-IDF and Arduino-ESP32 it uses the heap_caps API. Host builds (the
// unit tests) define PSRAM_ALLOCATOR_CUSTOM_BACKEND as the name of a header
// that provides the same functions in namespace psram::backend.

#pragma once

#include <cstddef>
#include <cstdint>

namespace psram {

/// Memory region a block is taken from.
enum class region : std::uint8_t {
    external,  ///< External PSRAM (SPIRAM).
    internal,  ///< Internal, byte-addressable RAM.
};

}  // namespace psram

#if defined(PSRAM_ALLOCATOR_CUSTOM_BACKEND)
#include PSRAM_ALLOCATOR_CUSTOM_BACKEND
#elif defined(ESP_PLATFORM)
#include <esp_heap_caps.h>

namespace psram::backend {

namespace detail {

/// Alignment that heap_caps_malloc() always provides.
inline constexpr std::size_t heap_alignment = 4;

[[nodiscard]] constexpr std::uint32_t caps_of(region r) noexcept {
    return r == region::external ? (MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
                                 : (MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
}

}  // namespace detail

/// Returns at least `bytes` bytes aligned to `alignment` from region `r`, or
/// nullptr when the region cannot satisfy the request.
[[nodiscard]] inline void* allocate(std::size_t bytes, std::size_t alignment, region r) noexcept {
    const std::uint32_t caps = detail::caps_of(r);
    if (alignment <= detail::heap_alignment) {
        return heap_caps_malloc(bytes, caps);
    }
    return heap_caps_aligned_alloc(alignment, bytes, caps);
}

/// Releases a block returned by allocate(). Accepts nullptr. heap_caps_free()
/// handles both regions and aligned blocks (ESP-IDF 4.4 and newer).
inline void deallocate(void* block) noexcept {
    heap_caps_free(block);
}

[[nodiscard]] inline std::size_t total_size(region r) noexcept {
    return heap_caps_get_total_size(detail::caps_of(r));
}

[[nodiscard]] inline std::size_t free_size(region r) noexcept {
    return heap_caps_get_free_size(detail::caps_of(r));
}

[[nodiscard]] inline std::size_t largest_free_block(region r) noexcept {
    return heap_caps_get_largest_free_block(detail::caps_of(r));
}

}  // namespace psram::backend

#else
#error "esp32-psram-allocator needs ESP-IDF or Arduino-ESP32 (unit tests define PSRAM_ALLOCATOR_CUSTOM_BACKEND)"
#endif
