# Minimal, compilable integration examples for external developers
# (Pitchfork examples/ · fcitx5 testing/ culture). Built in CI as a
# regression gate: a broken example is a red CI job.
enable_language(C)

# Walk the whole portable C API: engine -> session -> transliteration ->
# candidate selection -> commit. suborno_engine_demo "ami bhai"
add_executable(suborno_engine_demo
    engine_demo.c
)
target_link_libraries(suborno_engine_demo PRIVATE suborno_core)

# Smallest possible embedding: create, append, snapshot, destroy.
add_executable(suborno_c_api_example
    c_api_example.c
)
target_link_libraries(suborno_c_api_example PRIVATE suborno_core)

# Both examples go through the shared target configuration like every other
# target in the tree: warning flags, C++20/C standard properties, and —
# critically — the sanitizer LINK options. Without them a
# SUBORNO_ENABLE_SANITIZERS=ON build fails to link: the static libraries
# they consume (libsuborno_core.a) carry ASan/UBSan instrumentation whose
# runtime symbols (__asan_report_*, __ubsan_handle_*) are only provided when
# the final link line carries -fsanitize=address,undefined.
suborno_configure_target(suborno_engine_demo)
suborno_configure_target(suborno_c_api_example)
