# Four-level test taxonomy: unit, integration, windows (platform-specific)
# and portable fuzz-smoke coverage. Test names are part of the CI contract —
# suborno.unit, suborno.integration, suborno.windows.*.

find_package(GTest CONFIG REQUIRED)

add_executable(suborno_unit_tests
    unit/atomic_file_test.cpp
    unit/compat_env.cpp
    unit/compat_parity_test.cpp
    unit/bds_conformance_test.cpp
    unit/bds1520_official_corpus_test.cpp
    unit/project_smoke_test.cpp
    unit/c_api_test.cpp
    unit/candidate_engine_test.cpp
    unit/phonetic_parser_test.cpp
    unit/phonetic_input_session_test.cpp
    unit/layout_input_session_test.cpp
    unit/key_event_test.cpp
    unit/suborno_classic_layout_test.cpp
    unit/suborno_smart_layout_test.cpp
    unit/khipro_layout_test.cpp
    unit/phonetic_candidate_test.cpp
    unit/phonetic_tokenizer_test.cpp
    unit/unicode_test.cpp
    unit/ui_functional_audit_test.cpp
    unit/bijoy_test.cpp
    unit/engine_test.cpp
    unit/settings_test.cpp
    unit/linguistic_regression_test.cpp
    unit/layout_typing_regression_test.cpp
    unit/probhat_layout_test.cpp
    unit/national_layout_test.cpp
    unit/spell_checker_test.cpp
    unit/personal_dictionary_test.cpp
    unit/c_api_coverage_test.cpp
    unit/engine_coverage_test.cpp
    unit/layout_api_coverage_test.cpp
    unit/persistence_coverage_test.cpp
    unit/settings_coverage_test.cpp
    unit/unicode_coverage_test.cpp
    unit/coverage_layouts_test.cpp
    unit/coverage_dictionary_test.cpp
    unit/coverage_api_settings_test.cpp
    unit/coverage_unicode_test.cpp
    unit/tools_coverage_test.cpp
    unit/coverage_final_gaps_test.cpp
    unit/ui_strings_test.cpp
    unit/standalone_typing_controller_test.cpp
    unit/key_event_matrix_test.cpp
    unit/candidate_navigation_test.cpp
    unit/diagnostics_test.cpp
    unit/stability_soak_test.cpp
    unit/performance_budget_test.cpp
)
target_include_directories(suborno_unit_tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(suborno_unit_tests PRIVATE suborno_core GTest::gtest_main suborno_spell)
# The legacy reference IME compatibility engines resolve their data (ac.json,
# bn-khipro.mim) from the repository assets tree in tests.
target_compile_definitions(suborno_unit_tests PRIVATE
    SUBORNO_TEST_ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets")
if(MSVC AND SUBORNO_ENABLE_MSVC_ANALYSIS)
    suborno_mark_dependency_headers_external(suborno_unit_tests GTest::gtest)
    suborno_mark_dependency_headers_external(suborno_unit_tests GTest::gtest_main)
endif()
if(UNIX AND NOT APPLE)
    target_sources(suborno_unit_tests PRIVATE unit/platform_adapter_test.cpp)
    target_link_libraries(suborno_unit_tests PRIVATE suborno_linux_adapter)
endif()
if(UNIX AND NOT APPLE)
    # Corruption/injection scenarios for the CLI tools: they shell out to
    # python3 + sqlite3 and are therefore POSIX-only, mirroring the
    # platform_adapter registration above.
    target_sources(suborno_unit_tests PRIVATE unit/tools_unix_injection_test.cpp)
endif()
# CLI tools and examples are exercised end-to-end as subprocesses; the paths
# of the exact binaries built in this tree are handed to the tests as macros.
add_dependencies(suborno_unit_tests
    suborno_dictionary_import
    suborno_dictionary_bench
    suborno_engine_demo
    suborno_c_api_example
)
target_compile_definitions(suborno_unit_tests PRIVATE
    SUBORNO_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
    SUBORNO_IMPORT_TOOL="$<TARGET_FILE:suborno_dictionary_import>"
    SUBORNO_BENCH_TOOL="$<TARGET_FILE:suborno_dictionary_bench>"
    SUBORNO_ENGINE_DEMO="$<TARGET_FILE:suborno_engine_demo>"
    SUBORNO_C_API_EXAMPLE="$<TARGET_FILE:suborno_c_api_example>"
)
suborno_configure_test_target(suborno_unit_tests)
add_test(NAME suborno.unit COMMAND suborno_unit_tests
    "$<IF:$<CONFIG:Debug>,--gtest_filter=*-PerformanceBudget.*,--gtest_filter=*>"
)
# Full functional and soak coverage runs in both configurations. Timing
# budgets are meaningful only in optimized Release builds: MSVC Debug uses
# /Od, so PerformanceBudget runs once in Release. Expensive soak snapshots
# are checkpointed in the test itself.
#
# This is one CTest entry containing the entire GoogleTest executable (672
# cases across 74 suites). Its deterministic StabilitySoak cases are
# intentionally more expensive under MSVC /Od than in Release. Give this
# aggregate unit-test process its own bounded deadline; the workflow's
# --timeout value remains the shorter default for every other test.
#
# CMake's TIMEOUT test property takes precedence over CTest's default timeout.
set_tests_properties(suborno.unit PROPERTIES TIMEOUT 300)

add_executable(suborno_integration_tests integration/target_graph_smoke_test.cpp)
target_link_libraries(suborno_integration_tests PRIVATE suborno_core GTest::gtest_main)
suborno_configure_test_target(suborno_integration_tests)
if(MSVC AND SUBORNO_ENABLE_MSVC_ANALYSIS)
    suborno_mark_dependency_headers_external(suborno_integration_tests GTest::gtest)
    suborno_mark_dependency_headers_external(suborno_integration_tests GTest::gtest_main)
endif()
add_test(NAME suborno.integration COMMAND suborno_integration_tests)

if(WIN32)
    # WIL::WIL is an IMPORTED target created by find_package(wil) inside the
    # src/platform/windows subtree. IMPORTED targets are directory-scoped, so
    # they are invisible to this sibling directory: this tree must run its own
    # find_package before referencing the target (same pattern as
    # find_package(GTest) at the top of this file).
    find_package(wil CONFIG REQUIRED)
    add_executable(suborno_windows_tests
        windows/windows_build_smoke_test.cpp
        ../src/platform/windows/app_paths.cpp
        ../src/platform/windows/tsf/guids.cpp
        ../src/platform/windows/tsf/candidate_list_ui_element.cpp
    )
    target_link_libraries(suborno_windows_tests PRIVATE
        suborno_core
        suborno_windows_tsf
        GTest::gtest_main
        WIL::WIL
        Ole32.lib
        OleAut32.lib
        User32.lib
        Shell32.lib
        Uuid.lib
    )
    suborno_configure_test_target(suborno_windows_tests)
    if(MSVC AND SUBORNO_ENABLE_MSVC_ANALYSIS)
        suborno_mark_dependency_headers_external(suborno_windows_tests GTest::gtest)
        suborno_mark_dependency_headers_external(suborno_windows_tests GTest::gtest_main)
        suborno_mark_dependency_headers_external(suborno_windows_tests WIL::WIL)
    endif()
    add_test(NAME suborno.windows.tsf_contract COMMAND suborno_windows_tests)
    set_tests_properties(suborno.windows.tsf_contract PROPERTIES LABELS "windows;non-interactive")
    add_test(NAME suborno.windows.app_smoke COMMAND suborno_app --smoke-test)
    set_tests_properties(suborno.windows.app_smoke PROPERTIES
        LABELS "windows;non-interactive"
        TIMEOUT 30
    )
endif()

# One-command test runner: builds every test executable (plus the app the
# smoke test needs) in the configuration being built, then runs ctest with
# that same configuration. With the Visual Studio/MSVC generator this makes
# `cmake --build --preset windows-msvc-release --target suborno_run_tests`
# impossible to desync from the tested configuration (a Release build tree
# with `ctest -C Debug` reported the windows tests "Not Run" in the wild).
#
# The target is deliberately NOT named "run_tests": the Visual Studio
# generator already emits its own case-insensitively matching "RUN_TESTS"
# utility project, and two same-named projects break the solution with
# MSB5004 ("The solution file has two projects named \"run_tests\"") on
# --clean-first runs (observed on the real MSVC machine, 2026-11-25).
set(SUBORNO_TEST_TARGETS suborno_unit_tests suborno_integration_tests)
if(WIN32)
    list(APPEND SUBORNO_TEST_TARGETS suborno_app suborno_windows_tests)
endif()
if(CMAKE_CONFIGURATION_TYPES)
    set(SUBORNO_CTEST_CONFIG_ARG "-C" "$<CONFIG>")
else()
    set(SUBORNO_CTEST_CONFIG_ARG "") # single-config tree: ctest needs no -C
endif()
add_custom_target(suborno_run_tests
    COMMAND "${CMAKE_CTEST_COMMAND}" ${SUBORNO_CTEST_CONFIG_ARG} --output-on-failure
    DEPENDS ${SUBORNO_TEST_TARGETS}
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    USES_TERMINAL
    VERBATIM
)


# Portable fuzz-smoke driver: runs the shared fuzz targets (phonetic
# pipeline + public C API lifecycle) over the seed corpus plus deterministic
# mutations on every compiler and CI platform, with an alarm timeout. This
# is the always-on "fuzz smoke test" CI stage; the deep libFuzzer campaigns
# keep running in the fuzz.yml workflow.
add_executable(suborno_fuzz_smoke fuzz/fuzz_smoke_main.cpp)
target_link_libraries(suborno_fuzz_smoke PRIVATE suborno_core)
# The smoke driver links ASan/UBSan-instrumented core objects: the executable
# itself must carry the sanitizer flags so the matching runtime is linked in.
suborno_apply_build_options(suborno_fuzz_smoke)
add_test(NAME suborno.fuzz_smoke
    COMMAND suborno_fuzz_smoke "${PROJECT_SOURCE_DIR}/tests/fuzz/corpus")
set_tests_properties(suborno.fuzz_smoke PROPERTIES TIMEOUT 180)
# Keep the registered smoke test executable available to the umbrella runner.
add_dependencies(suborno_run_tests suborno_fuzz_smoke)
