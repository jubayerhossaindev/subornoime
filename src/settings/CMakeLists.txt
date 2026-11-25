# Settings model, INI serialization and the settings store.
add_library(suborno_settings STATIC
    settings.cpp
)
target_include_directories(suborno_settings PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(suborno_settings PRIVATE suborno_persistence)
suborno_configure_target(suborno_settings)
