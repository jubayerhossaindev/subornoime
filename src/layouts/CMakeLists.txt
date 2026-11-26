# Session-state layer shared by every keyboard layout, then one target per
# layout family — adding a new keyboard layout means adding one subdirectory.
add_library(suborno_layout_sessions STATIC
    layout_input_session.cpp
)
target_include_directories(suborno_layout_sessions PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(suborno_layout_sessions PUBLIC
    suborno_layout_api
    suborno_unicode
    suborno_dictionary
    suborno_candidates
)
suborno_configure_target(suborno_layout_sessions)

add_subdirectory(compat)
add_subdirectory(phonetic)
add_subdirectory(english)
add_subdirectory(khipro)
add_subdirectory(probhat)
add_subdirectory(national)
