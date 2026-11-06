#pragma once

#include <cstdint>
#include <vector>

namespace lzh {

// Decompresses a complete LZH1 container.  Throws std::runtime_error on
// malformed or truncated input.
std::vector<uint8_t> decompressData(const std::vector<uint8_t>& in);

}  // namespace lzh
