// libFuzzer driver for the public C API boundary (Clang-only target).
#include <cstddef>
#include <cstdint>

#include "fuzz_targets.hpp"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, std::size_t size) {
    suborno::fuzz::run_c_api_target(data, size);
    return 0;
}
