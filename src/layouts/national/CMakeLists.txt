# Bangladesh national keyboard layout.
add_library(suborno_national STATIC
    national_layout.cpp
)
target_include_directories(suborno_national PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(suborno_national PUBLIC suborno_layout_api suborno_unicode)
suborno_configure_target(suborno_national)
