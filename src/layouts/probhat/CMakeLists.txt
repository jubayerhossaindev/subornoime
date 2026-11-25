# Probhat fixed keyboard layout.
add_library(suborno_probhat STATIC
    probhat_layout.cpp
)
target_include_directories(suborno_probhat PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(suborno_probhat PUBLIC suborno_layout_api suborno_unicode)
suborno_configure_target(suborno_probhat)
