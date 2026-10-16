# Changelog

## 1.0.0 (2026-10-16)

First release as a standalone library. It replaces the earlier single-header `stdpsram` code.

- `psram::allocator` with the `external_only` and `prefer_external` policies
- Aliases for the standard containers and strings, with a hash for `psram::string` keys
- `psram::make_unique` and `psram::unique_ptr`
- `std::pmr` resources for toolchains with `<memory_resource>`
- Host unit tests with a simulated heap, and CI builds for ESP-IDF and PlatformIO
