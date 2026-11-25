# Candidate generation, ranking and language intelligence.
add_library(suborno_candidates STATIC
    candidate_engine.cpp
    language_intelligence.cpp
)
target_include_directories(suborno_candidates PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(suborno_candidates PUBLIC
    suborno_layout_api
    suborno_unicode
    suborno_dictionary
    suborno_spell
)
suborno_configure_target(suborno_candidates)
