# C++20 Standards

The single permitted language standard for the whole repository is **ISO C++20**.

## Enforcement

The root `CMakeLists.txt` locks the standard for every target:

```cmake
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
```

`CMAKE_CXX_STANDARD_REQUIRED` fails the configure step when a compiler cannot
provide C++20; `CMAKE_CXX_EXTENSIONS OFF` disables GNU/MSVC extensions so the
sources stay standard-conforming. The CI matrix (`.github/workflows/ci.yml`)
compiles the core with Clang 23 and GCC 13 on Ubuntu and with MSVC v143
(Visual Studio 17 2022) on Windows, all in C++20 mode.

## Permitted C++20 features

Used in this codebase or available when a real need appears:

- Concepts and `requires` expressions.
- Ranges (`std::views::…`) where they beat plain loops in clarity.
- Coroutines (not used today; no platform integration requires them).
- Three-way comparison (`<=>`) for value-ordered types.
- Designated initializers — used by the layout/candidate construction code.
- `std::span`, `std::string_view`, `std::optional`, `std::variant`.
- `std::bit_cast`, `std::source_location` (diagnostics), `std::jthread` and
  `std::stop_token` for cancellable workers.
- `std::format` once the toolchain floor (MSVC v143) is confirmed per call site.
- `constexpr` / `consteval` / `constinit`, structured bindings, `std::array`,
  `std::filesystem`, move semantics.

## Forbidden

- Any C++23 or later language or library feature (`std::expected`,
  `std::print`, deducing `this`, `std::mdspan`, …).
- Compiler extensions (GNU/MSVC) — C++20 standard C++ only.
- Non-standard target configurations.

Before using any feature, verify that every compiler in the CI matrix supports
it; when in doubt, do not use it.

## Compiler floor

| Compiler | Version | Role |
|---|---|---|
| MSVC (`cl.exe`) | v143 / VS 17 2022 | Primary Windows build (TSF DLL + shell) |
| Clang | 23 | Ubuntu development + CI |
| GCC | 13.3 | Ubuntu compatibility CI |
