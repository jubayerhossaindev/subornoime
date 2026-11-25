# Crash-safe file persistence primitives.
add_library(suborno_persistence STATIC
    atomic_file.cpp
)
target_include_directories(suborno_persistence PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(suborno_persistence PUBLIC unofficial::sqlite3::sqlite3)
suborno_configure_target(suborno_persistence)
