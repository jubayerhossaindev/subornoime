cmake_minimum_required(VERSION 3.25)

if(POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
endif()

project(subornoime VERSION 0.1.0 LANGUAGES CXX)

option(BUILD_TESTING "Build SUBORNO test executables" OFF)
option(SUBORNO_BUILD_BENCHMARKS "Build optional Google Benchmark executables" OFF)
option(SUBORNO_ENABLE_MSVC_ANALYSIS "Enable MSVC static analysis with /analyze" ON)
option(SUBORNO_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(SUBORNO_BUILD_FUZZING "Build optional libFuzzer targets" OFF)
option(SUBORNO_ENABLE_MSVC_CCACHE "Enable the MSVC compilation cache when ccache is available" ON)

# Visual Studio's MSBuild generator uses CLToolExe/CLToolPath rather than
# CMAKE_CXX_COMPILER_LAUNCHER. ccache documents this masquerading setup for
# MSVC; /Z7 is required instead of /Zi, and forced includes/PCH are avoided.
# The cache itself lives in RUNNER_TEMP (not the build tree) on CI.
if(WIN32 AND MSVC AND SUBORNO_ENABLE_MSVC_CCACHE)
    set(_suborno_ccache_executable "$ENV{SUBORNO_CCACHE_EXECUTABLE}")
    if(_suborno_ccache_executable AND EXISTS "${_suborno_ccache_executable}")
        set(_suborno_ccache_proxy_dir "${CMAKE_BINARY_DIR}/ccache-proxy")
        file(MAKE_DIRECTORY "${_suborno_ccache_proxy_dir}")
        file(COPY_FILE
            "${_suborno_ccache_executable}"
            "${_suborno_ccache_proxy_dir}/cl.exe"
            ONLY_IF_DIFFERENT
        )

        set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
            "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>"
        )
        set(CMAKE_VS_GLOBALS
            "CLToolExe=cl.exe"
            "CLToolPath=${_suborno_ccache_proxy_dir}"
            "UseMultiToolTask=true"
        )
        message(STATUS "MSVC compilation cache enabled through ccache: ${_suborno_ccache_executable}")
    endif()
endif()

include("${PROJECT_SOURCE_DIR}/cmake/modules/SubornoBuildOptions.cmake")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Keep every target's outputs at the top-level build directory, reproducing
# the original single-CMakeLists layout that the packaging contract depends
# on: packaging/windows/suborno_installer.iss reads
# {#MyBuildDir}\Release\Suborno.exe and {#MyBuildDir}\Release\
# suborno_windows_tsf.dll, and .github/workflows/release.yml reads
# suborno_version.iss.inc from the same root. Multi-config generators (VS)
# append the $<CONFIG> subdirectory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

find_package(ICU REQUIRED COMPONENTS uc i18n)
# The vcpkg toolchain provides unofficial-sqlite3; fall back to the system
# SQLite3 package (Debian/Fedora etc.) when building without vcpkg.
find_package(unofficial-sqlite3 CONFIG QUIET)
if(NOT unofficial-sqlite3_FOUND)
    find_package(SQLite3 REQUIRED)
    if(TARGET SQLite3::SQLite3)
        add_library(unofficial::sqlite3::sqlite3 ALIAS SQLite3::SQLite3)
    else()
        add_library(unofficial::sqlite3::sqlite3 ALIAS SQLite::SQLite3)
    endif()
endif()

# Portable core domains, in dependency order. Every directory owns the
# definition of its own targets (LLVM per-project · systemd per-component
# layout): module boundaries are visible in the build and merge conflicts
# stay inside the owning directory.
add_subdirectory(src/unicode)
add_subdirectory(src/layout-api)
add_subdirectory(src/dictionary)
add_subdirectory(src/spell)
add_subdirectory(src/candidates)
add_subdirectory(src/persistence)
add_subdirectory(src/settings)
add_subdirectory(src/layouts)
add_subdirectory(src/core)
add_subdirectory(src/platform)
add_subdirectory(src/tools)
add_subdirectory(examples)

# google/benchmark suites (per-release performance numbers). Optional: the
# build works without the dependency and the CI latency fences live in
# tests/unit/performance_budget_test.cpp, which has no external dependency.
option(SUBORNO_BUILD_BENCHMARKS "Build the google/benchmark suites" OFF)
if(SUBORNO_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

# Optional Doxygen documentation target: configures docs/api/Doxyfile.in into
# the build tree and generates the public-API HTML documentation. Requires
# doxygen on PATH; the rest of the build works without it.
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    set(SUBORNO_DOXYFILE_IN "${PROJECT_SOURCE_DIR}/docs/api/Doxyfile.in")
    set(SUBORNO_DOXYFILE_OUT "${CMAKE_BINARY_DIR}/docs/Doxyfile")
    configure_file("${SUBORNO_DOXYFILE_IN}" "${SUBORNO_DOXYFILE_OUT}" @ONLY)
    add_custom_target(
        docs
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/docs/api"
        COMMAND ${DOXYGEN_EXECUTABLE} "${SUBORNO_DOXYFILE_OUT}"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/docs"
        COMMENT "Generating public API documentation with Doxygen"
        VERBATIM
    )
endif()
