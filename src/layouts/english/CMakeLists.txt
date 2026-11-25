# English passthrough layout: a selectable, persistable layout id whose keys
# the text service never intercepts.
add_library(suborno_english_layout STATIC
    english_layout.cpp
)
target_include_directories(suborno_english_layout PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_include_directories(suborno_english_layout PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(suborno_english_layout PUBLIC
    suborno_layout_api
    suborno_layout_sessions
)
suborno_configure_target(suborno_english_layout)
