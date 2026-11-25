# SQLite-backed dictionary store and lookup.
add_library(suborno_dictionary STATIC
    dictionary.cpp
)
target_include_directories(suborno_dictionary PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(suborno_dictionary PUBLIC unofficial::sqlite3::sqlite3)
suborno_configure_target(suborno_dictionary)
