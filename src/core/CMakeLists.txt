# Engine facade, ABI/project metadata and the portable C API.
add_library(suborno_core STATIC
    project.cpp
    engine.cpp
    c_api.cpp
    ui_strings.cpp
    diagnostics.cpp
)
target_include_directories(suborno_core PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(suborno_core
    PUBLIC
        suborno_unicode
        suborno_layout_api
        suborno_phonetic
        suborno_english_layout
        suborno_khipro
        suborno_probhat
        suborno_national
        suborno_candidates
        suborno_dictionary
        suborno_persistence
        suborno_layout_registry
        suborno_layout_sessions
        suborno_settings
)
target_compile_definitions(suborno_core PUBLIC SUBORNO_API_STATIC)
suborno_configure_target(suborno_core)
