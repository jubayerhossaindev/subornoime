# Khipro fixed keyboard layout.
add_library(suborno_khipro STATIC
    khipro_layout.cpp
)
target_include_directories(suborno_khipro PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(suborno_khipro PUBLIC
    suborno_layout_api
    suborno_unicode
    suborno_layout_sessions
    suborno_compat
)
suborno_configure_target(suborno_khipro)
