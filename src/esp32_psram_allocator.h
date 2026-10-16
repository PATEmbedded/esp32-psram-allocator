// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Pourya Afshintabar
//
// Single include for the library: allocators, containers and, when the
// standard library provides <memory_resource>, the std::pmr resources.

#pragma once

#include "psram/allocator.hpp"
#include "psram/containers.hpp"

#if __has_include(<memory_resource>)
#include "psram/memory_resource.hpp"
#endif
