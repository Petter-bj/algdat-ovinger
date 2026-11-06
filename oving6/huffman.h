#pragma once

#include <cstdint>
#include <vector>

#include "bit_io.h"

namespace lzh {

constexpr int kMaxCodeLen = 15;

struct HuffmanTable {
    std::vector<uint8_t> length;  // code length per symbol, 0 = unused
    std::vector<uint32_t> code;   // canonical code per symbol
};

// Builds length-limited canonical Huffman code lengths.
HuffmanTable buildHuffman(const std::vector<uint32_t>& freq, int maxLen);

class HuffmanDecoder {
public:
    // Rebuilds the canonical decoding tables from stored code lengths.
    void init(const std::vector<uint8_t>& lengths);

    // Reads one symbol, advancing the bit reader.
    int decode(BitReader& reader) const;

private:
    std::vector<uint8_t> length_;
    std::vector<uint16_t> count_;
    std::vector<uint32_t> firstCode_;
    std::vector<int> firstIndex_;
    std::vector<int> symbols_;
    int maxLen_ = 0;
};

}  // namespace lzh
