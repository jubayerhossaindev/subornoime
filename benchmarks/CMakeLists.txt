# Named benchmark suites (Redis runtest culture): per-release performance
# regression tracking over the hot paths — dictionary I/O, engine sessions,
# and the cross-domain infrastructure. Included from the root CMakeLists only
# when SUBORNO_BUILD_BENCHMARKS=ON, so the plain build never requires the
# google/benchmark dependency.
find_package(benchmark CONFIG QUIET)
if(NOT benchmark_FOUND)
    message(STATUS
        "google/benchmark not found — SUBORNO_BUILD_BENCHMARKS disabled "
        "(CI latency fences live in tests/unit/performance_budget_test.cpp)")
    return()
endif()

add_executable(suborno_benchmarks
    infrastructure_benchmark.cpp
    dictionary_benchmark.cpp
    engine_benchmark.cpp
)
target_link_libraries(suborno_benchmarks PRIVATE suborno_core benchmark::benchmark_main)
suborno_configure_target(suborno_benchmarks)

# When ctest is enabled the engine hot-path benchmark runs as its own suite:
# short, filtered and repeated on every release tag, with the full numbers
# captured in the test output for the performance report.
if(BUILD_TESTING)
    add_test(NAME suborno.benchmarks.engine COMMAND suborno_benchmarks
        --benchmark_filter=BenchmarkEngineSession --benchmark_min_time=0.1s)
    set_tests_properties(suborno.benchmarks.engine PROPERTIES TIMEOUT 300)
endif()
