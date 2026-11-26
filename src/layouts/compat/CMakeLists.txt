# Compatibility engines implementing the legacy reference IME typing behaviour inside
# Suborno's layout architecture (Suborno Classic, Suborno Smart, Khipro
# keymap, autocorrect dictionary). These are Suborno implementations of the
# upstream GPL-3.0-or-later behaviour contracts; see LICENSE and
# reports/ for the provenance and the parity evidence.
add_library(suborno_compat STATIC
    compat_dictionary_loader.cpp
    compat_candidates.cpp
    compat_classic_engine.cpp
    compat_smart_engine.cpp
    compat_khipro.cpp
    compat_mim_engine.cpp
)
target_include_directories(suborno_compat PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(suborno_compat PUBLIC suborno_layout_api)
suborno_configure_target(suborno_compat)
