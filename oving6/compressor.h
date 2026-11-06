#pragma once

#include <cstdint>
#include <vector>

namespace lzh {

// Compresses the whole input into a self-contained LZH1 container.  The
// encoder parameter choices are fixed at compile time; the decoder needs only
// the resulting bytes.
std::vector<uint8_t> compressData(const std::vector<uint8_t>& input);

}  // namespace lzh
