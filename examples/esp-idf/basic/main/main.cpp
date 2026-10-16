// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Pourya Afshintabar
//
// Puts a camera-sized frame buffer, a sensor table, a reading history and a
// 64 KB work buffer in PSRAM, and logs how much RAM is left in each region.

#include <array>
#include <cstdint>

#include "esp32_psram_allocator.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

namespace {

constexpr const char* kTag = "psram_example";

struct Reading {
    std::uint32_t time_ms;
    float value;
};

void log_free_memory(const char* stage) {
    ESP_LOGI(kTag, "%-16s internal free: %7u bytes, PSRAM free: %8u bytes", stage,
             static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)),
             static_cast<unsigned>(psram::free_bytes()));
}

}  // namespace

extern "C" void app_main(void) {
    if (!psram::available()) {
        ESP_LOGE(kTag, "No PSRAM found: use a module with PSRAM and enable CONFIG_SPIRAM");
        return;
    }
    log_free_memory("start");

    // A 320 x 240 RGB565 frame is 150 KB: too much to keep next to the Wi-Fi stack.
    psram::vector<std::uint16_t> frame(320 * 240, 0);

    psram::map<int, psram::string> sensors;
    sensors.emplace(1, "supply air temperature");
    sensors.emplace(2, "return air temperature");

    psram::deque<Reading> history;
    for (std::uint32_t i = 0; i < 1000; ++i) {
        history.push_back({i * 100, 20.0f + static_cast<float>(i % 10) * 0.1f});
    }

    auto work_buffer = psram::make_unique<std::array<std::uint8_t, 64 * 1024>>();
    work_buffer->fill(0xA5);

    log_free_memory("after allocation");
    ESP_LOGI(kTag, "frame: %u pixels, sensors: %u, readings: %u, work buffer: %u bytes",
             static_cast<unsigned>(frame.size()), static_cast<unsigned>(sensors.size()),
             static_cast<unsigned>(history.size()), static_cast<unsigned>(work_buffer->size()));
}
