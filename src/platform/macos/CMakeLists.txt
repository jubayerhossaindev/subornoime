# macOS platform adapter.
add_library(suborno_macos_adapter STATIC
    macos_adapter.cpp
)
target_include_directories(suborno_macos_adapter PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(suborno_macos_adapter PUBLIC suborno_core)
suborno_configure_target(suborno_macos_adapter)
