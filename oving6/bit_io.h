#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "error.h"

namespace lzh {

// Bits are filled least-significant-bit first.  Huffman codes are emitted
// most-significant-bit first, one bit at a time, so the decoder sees exactly
// the canonical bit string.
class BitWriter {
public:
    explicit BitWriter(std::vector<uint8_t>& out) : out_(out) {}

    void putBit(uint32_t bit) {
        buffer_ |= static_cast<uint64_t>(bit & 1) << used_;
        if (++used_ == 64) flushWord();
    }

    void putBits(uint32_t value, int count) {
        for (int i = 0; i < count; ++i) putBit((value >> i) & 1u);
    }

    void putCode(uint32_t code, int length) {
        for (int i = length - 1; i >= 0; --i) putBit((code >> i) & 1u);
    }

    // Pads the current byte with zero bits and emits all complete bytes.
    void align() {
        while ((used_ % 8) != 0) putBit(0);
        flushWord();
    }

private:
    void flushWord() {
        while (used_ >= 8) {
            out_.push_back(static_cast<uint8_t>(buffer_ & 0xFF));
            buffer_ >>= 8;
            used_ -= 8;
        }
    }

    std::vector<uint8_t>& out_;
    uint64_t buffer_ = 0;
    int used_ = 0;
};

class BitReader {
public:
    BitReader(const uint8_t* data, size_t size) : data_(data), size_(size) {}

    uint32_t readBit() {
        if (bitPos_ >= size_ * 8) fail("truncated bit stream");
        uint32_t bit = (data_[bitPos_ >> 3] >> (bitPos_ & 7)) & 1u;
        ++bitPos_;
        return bit;
    }

    uint32_t readBits(int count) {
        uint32_t value = 0;
        for (int i = 0; i < count; ++i) value |= readBit() << i;
        return value;
    }

    size_t bitsRemaining() const { return size_ * 8 - bitPos_; }
    size_t bitsUsed() const { return bitPos_; }

    // True when the bits from the current position to the next byte boundary
    // are all zero (used after a self-delimiting bit payload).
    bool zeroPaddingToByteBoundary() const {
        size_t end = ((bitPos_ + 7) / 8) * 8;
        if (end > size_ * 8) return false;
        for (size_t bit = bitPos_; bit < end; ++bit) {
            if (((data_[bit >> 3] >> (bit & 7)) & 1u) != 0) return false;
        }
        return true;
    }

    // After the last token of a block only byte padding may remain.
    bool onlyZeroPadding() const {
        if (bitPos_ > size_ * 8) return false;
        if (size_ * 8 - bitPos_ >= 8) return false;
        for (size_t bit = bitPos_; bit < size_ * 8; ++bit) {
            if (((data_[bit >> 3] >> (bit & 7)) & 1u) != 0) return false;
        }
        return true;
    }

private:
    const uint8_t* data_;
    size_t size_;
    size_t bitPos_ = 0;
};

}  // namespace lzh
