# Linux platform adapter.
add_library(suborno_linux_adapter STATIC
    linux_adapter.cpp
)
target_include_directories(suborno_linux_adapter PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(suborno_linux_adapter PUBLIC suborno_core)
suborno_configure_target(suborno_linux_adapter)
