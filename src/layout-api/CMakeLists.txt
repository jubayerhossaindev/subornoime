# Public layout interface plus the registry and declarative-layout helpers.
add_library(suborno_layout_api INTERFACE)
target_include_directories(suborno_layout_api INTERFACE "${PROJECT_SOURCE_DIR}/include")

add_library(suborno_layout_registry STATIC
    layout_registry.cpp
    declarative_layout.cpp
)
target_include_directories(suborno_layout_registry PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(suborno_layout_registry PUBLIC suborno_layout_api suborno_unicode)
suborno_configure_target(suborno_layout_registry)
