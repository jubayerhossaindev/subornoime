# Compatibility engines implementing the BornoTSF typing behaviour inside
# Suborno's layout architecture (Borno Phonetic, Avro Phonetic, Khipro
# keymap, autocorrect dictionary). These are Suborno implementations of the
# upstream GPL-3.0-or-later behaviour contracts; see LICENSE and
# reports/ for the provenance and the parity evidence.
add_library(suborno_borno_compat STATIC
    borno_compat_dictionary_loader.cpp
    borno_compat_candidates.cpp
    borno_compat_borno_engine.cpp
    borno_compat_avro_engine.cpp
    borno_compat_khipro.cpp
    borno_compat_mim_engine.cpp
)
target_include_directories(suborno_borno_compat PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(suborno_borno_compat PUBLIC suborno_layout_api)
suborno_configure_target(suborno_borno_compat)
