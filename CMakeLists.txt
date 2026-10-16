# Inside an ESP-IDF build the repository is a header-only component.
if(ESP_PLATFORM)
    idf_component_register(INCLUDE_DIRS "src")
    return()
endif()

cmake_minimum_required(VERSION 3.16)
project(esp32_psram_allocator VERSION 1.0.0 LANGUAGES CXX)

add_library(psram_allocator INTERFACE)
add_library(psram::allocator ALIAS psram_allocator)
target_include_directories(psram_allocator INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(psram_allocator INTERFACE cxx_std_17)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    option(PSRAM_ALLOCATOR_BUILD_TESTS "Build the host unit tests" ON)
    option(PSRAM_ALLOCATOR_SANITIZE "Build the unit tests with AddressSanitizer and UBSan" OFF)
    if(PSRAM_ALLOCATOR_BUILD_TESTS)
        enable_testing()
        add_subdirectory(test)
    endif()
endif()
