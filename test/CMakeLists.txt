include(FetchContent)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

# Mark GoogleTest headers as system headers so -Werror applies to our code only.
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.25)
    set(googletest_system SYSTEM)
endif()

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz
    URL_HASH SHA256=65fab701d9829d38cb77c14acdc431d2108bfdbf8979e40eb8ae567edf10b27c
    ${googletest_system}
)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(psram_allocator_tests
    allocator_test.cpp
    containers_test.cpp
    memory_resource_test.cpp
)
target_include_directories(psram_allocator_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(psram_allocator_tests PRIVATE PSRAM_ALLOCATOR_CUSTOM_BACKEND="host_backend.hpp")
target_link_libraries(psram_allocator_tests PRIVATE psram::allocator GTest::gtest_main)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(psram_allocator_tests PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Werror)
    if(PSRAM_ALLOCATOR_SANITIZE)
        target_compile_options(psram_allocator_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(psram_allocator_tests PRIVATE -fsanitize=address,undefined)
    endif()
endif()

include(GoogleTest)
gtest_discover_tests(psram_allocator_tests)
