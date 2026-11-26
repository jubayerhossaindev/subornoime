# Phonetic layout family: Suborno/Classic/Smart mappings, parser, tokenizer
# and the phonetic input session.
add_library(suborno_phonetic STATIC
    suborno_phonetic_layout.cpp
    suborno_classic_layout.cpp
    suborno_smart_layout.cpp
    rule_based_phonetic_layout.cpp
    phonetic_input_session.cpp
    phonetic_parser.cpp
    phonetic_tokenizer.cpp
)
target_include_directories(suborno_phonetic PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_include_directories(suborno_phonetic PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(suborno_phonetic PUBLIC
    suborno_layout_api
    suborno_unicode
    suborno_layout_sessions
    suborno_candidates
    suborno_compat
)
suborno_configure_target(suborno_phonetic)
