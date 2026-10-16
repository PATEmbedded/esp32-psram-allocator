// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Pourya Afshintabar
//
// Arduino version of the basic example: a frame buffer and a sensor table in PSRAM.

#include <Arduino.h>
#include <esp32_psram_allocator.h>

void setup() {
    Serial.begin(115200);
    if (!psram::available()) {
        Serial.println("No PSRAM found: use a board with PSRAM and build with -DBOARD_HAS_PSRAM");
        return;
    }

    const size_t free_before = psram::free_bytes();

    psram::vector<uint16_t> frame(320 * 240, 0);
    psram::map<int, psram::string> sensors{{1, "supply air temperature"}, {2, "return air temperature"}};

    Serial.printf("frame: %u pixels, sensors: %u\n", static_cast<unsigned>(frame.size()),
                  static_cast<unsigned>(sensors.size()));
    Serial.printf("PSRAM used: %u bytes, still free: %u bytes\n",
                  static_cast<unsigned>(free_before - psram::free_bytes()),
                  static_cast<unsigned>(psram::free_bytes()));
}

void loop() {}
