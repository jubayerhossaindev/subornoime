# Spell correction over the dictionary store.
add_library(suborno_spell STATIC
    spell_checker.cpp
)
target_include_directories(suborno_spell PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(suborno_spell PUBLIC suborno_layout_api suborno_dictionary)
suborno_configure_target(suborno_spell)
