#pragma once

#include <Windows.h>
#include <new>

namespace suborno::platform::windows::com {

// A COM entry point must never let a C++ exception cross the binary
// interface: callers outside the module unwind with a different runtime
// contract and the process terminates. Every COM method that executes
// project C++ code routes its body through this guard, which converts an
// escaping exception into the matching COM error code.
//
// Call sites wrap the method body in a lambda captured by reference. That
// capture is safe here by construction: the lambda runs synchronously on
// the same stack frame and never outlives the method call, so no captured
// reference is used after its lifetime ends.
template <typename Operation>
HRESULT run_inside_com_boundary(Operation&& operation) noexcept {
    try {
        return operation();
    } catch (const std::bad_alloc&) {
        return E_OUTOFMEMORY;
    } catch (...) {
        return E_UNEXPECTED;
    }
}

}  // namespace suborno::platform::windows::com
