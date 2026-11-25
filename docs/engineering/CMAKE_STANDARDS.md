# CMake Standards

Modern, target-based CMake. Every module directory owns its own
`CMakeLists.txt`; the root file only sets policy and adds subdirectories.

## Policy (root `CMakeLists.txt`)

```cmake
cmake_minimum_required(VERSION 3.25)
project(subornoime VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
```

Options: `BUILD_TESTING`, `SUBORNO_BUILD_BENCHMARKS`, `SUBORNO_ENABLE_MSVC_ANALYSIS`,
`SUBORNO_ENABLE_SANITIZERS`, `SUBORNO_BUILD_FUZZING`.

## Target-based rules

- Dependencies are declared with `target_link_libraries` (PRIVATE/PUBLIC/INTERFACE),
  include paths with `target_include_directories`, features with
  `target_compile_features(... cxx_std_20)` — never global
  `include_directories`/`add_definitions`.
- Shared build options live in `cmake/modules/SubornoBuildOptions.cmake`
  (warning levels, MSVC `/analyze` wiring, the forced-include
  `cmake/msvc_test_c6326_suppress.h`, test-target helpers).
- Third parties come from the vcpkg manifest (`vcpkg.json`: ICU, SQLite, WIL,
  GTest, Google Benchmark) with a system-package fallback for SQLite.
- Output discipline: all runtime/library outputs go to the top-level build dir
  so the Windows installer and release workflow find them
  (`CMAKE_RUNTIME_OUTPUT_DIRECTORY` etc.).

## Documentation target

`find_package(Doxygen)` + `configure_file(docs/api/Doxyfile.in … @ONLY)` +
custom `docs` target (see DOXYGEN_STANDARDS.md).

## Naming

Targets are named after their module: `suborno_unicode`, `suborno_layout_api`,
`suborno_dictionary`, `suborno_spell`, `suborno_candidates`,
`suborno_persistence`, `suborno_settings`, `suborno_core`,
`suborno_windows_tsf`, `suborno_app`, plus `suborno_unit_tests`,
`suborno_integration_tests`. Options keep the `SUBORNO_` prefix.

## Validation

- `cmake --preset <name>` presets: `ubuntu-dev`, `ubuntu-ci`, `ubuntu-bench`,
  `ubuntu-gcc-ci`, `windows-msvc`, `windows-msvc-release` (see
  `CMakePresets.json`).
- CI configures every preset; a configure failure fails the pipeline.
