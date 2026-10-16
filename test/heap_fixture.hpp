// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Pourya Afshintabar

#pragma once

#include <gtest/gtest.h>

#include "psram/backend.hpp"

/// Gives every test a fresh simulated heap and fails the test if it leaks.
class HeapTest : public ::testing::Test {
protected:
    void SetUp() override {
        psram::backend::heap().reset();
    }

    void TearDown() override {
        EXPECT_TRUE(psram::backend::heap().live.empty()) << "the test leaked memory";
    }

    static void set_capacity(std::size_t external_capacity,
                             std::size_t internal_capacity = psram::backend::default_internal_capacity) {
        psram::backend::heap().reset(external_capacity, internal_capacity);
    }

    static const psram::backend::region_state& external() {
        return psram::backend::heap().external;
    }

    static const psram::backend::region_state& internal() {
        return psram::backend::heap().internal;
    }
};
