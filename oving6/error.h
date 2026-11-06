#pragma once

#include <stdexcept>
#include <string>

namespace lzh {

// Single error path for malformed input and internal invariants.  The CLI maps
// this exception to exit status 2.
[[noreturn]] inline void fail(const std::string& message) {
    throw std::runtime_error(message);
}

}  // namespace lzh
