# Command-line tools built on the portable core.
# Portable dictionary import tool: suborno_dictionary_import <db.sqlite> <words.tsv>
add_executable(suborno_dictionary_import
    dictionary_import_main.cpp
)
target_link_libraries(suborno_dictionary_import PRIVATE suborno_dictionary suborno_unicode)
suborno_configure_target(suborno_dictionary_import)

# Million-scale dictionary benchmark: suborno_dictionary_bench <db> [count] [queries]
add_executable(suborno_dictionary_bench
    dictionary_bench_main.cpp
)
target_link_libraries(suborno_dictionary_bench PRIVATE suborno_dictionary)
suborno_configure_target(suborno_dictionary_bench)
