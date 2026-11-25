# Bengali Unicode classification and grapheme segmentation (ICU-backed),
# plus the portable Bijoy (legacy ANSI) encoder for ANSI-mode commits.
add_library(suborno_unicode STATIC
    bengali.cpp
    bijoy.cpp
)
target_include_directories(suborno_unicode PUBLIC "${PROJECT_SOURCE_DIR}/include")
# bengali.cpp uses ICU BreakIterator (grapheme segmentation), which lives in
# the i18n library — linking only ICU::uc fails at link time.
target_link_libraries(suborno_unicode PUBLIC ICU::uc ICU::i18n)
suborno_configure_target(suborno_unicode)
