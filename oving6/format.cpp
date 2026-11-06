#include "format.h"

#include <algorithm>
#include <array>
#include <iterator>

#include "bit_io.h"
#include "huffman.h"

namespace lzh {

const uint8_t kMagic[4] = {'L', 'Z', 'H', '1'};

void appendVarint(std::vector<uint8_t>& out, uint64_t value) {
    while (value >= 0x80) {
        out.push_back(static_cast<uint8_t>(value) | 0x80);
        value >>= 7;
    }
    out.push_back(static_cast<uint8_t>(value));
}

uint64_t readVarint(const std::vector<uint8_t>& in, size_t& pos) {
    uint64_t value = 0;
    for (int index = 0; index < 10; ++index) {
        if (pos >= in.size()) fail("truncated varint");
        uint8_t byte = in[pos++];
        if (index == 9) {
            if ((byte & 0x80) != 0) fail("varint longer than 10 bytes");
            if (byte > 1) fail("varint overflow");
            value |= static_cast<uint64_t>(byte) << 63;
            return value;
        }
        value |= static_cast<uint64_t>(byte & 0x7F) << (7 * index);
        if ((byte & 0x80) == 0) return value;
    }
    fail("unreachable varint state");
}

LengthModel::LengthModel() {
    const uint16_t bases[29] = {
        3, 4, 5, 6, 7, 8, 9, 10,
        11, 13, 15, 17,
        19, 23, 27, 31,
        35, 43, 51, 59,
        67, 83, 99, 115,
        131, 163, 195, 227,
        258};
    const uint8_t extras[29] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        1, 1, 1, 1,
        2, 2, 2, 2,
        3, 3, 3, 3,
        4, 4, 4, 4,
        5, 5, 5, 5,
        0};
    base.assign(std::begin(bases), std::end(bases));
    extra.assign(std::begin(extras), std::end(extras));

    uint32_t length = 259;
    while (length <= kMaxMatch) {
        int width = 0;
        while ((uint32_t(1) << (width + 1)) <= length) ++width;
        int extraBits = std::max(0, width - 2);  // four slots per octave
        base.push_back(static_cast<uint16_t>(length));
        extra.push_back(static_cast<uint8_t>(extraBits));
        length += (1u << extraBits);
    }

    symbol.assign(kMaxMatch + 1, 0);
    extraValue.assign(kMaxMatch + 1, 0);
    for (size_t i = 0; i < base.size(); ++i) {
        uint32_t span = 1u << extra[i];
        for (uint32_t v = 0; v < span; ++v) {
            uint32_t len = base[i] + v;
            if (len <= kMaxMatch) {
                symbol[len] = static_cast<uint8_t>(i);
                extraValue[len] = static_cast<uint16_t>(v);
            }
        }
    }
}

const LengthModel& lengthModel() {
    static const LengthModel model;
    return model;
}

namespace {

constexpr int kDistSlotsPerOctave = 2;

}  // namespace

DistanceModel::DistanceModel() {
    base = {1, 2, 3, 4};
    extra = {0, 0, 0, 0};
    int logSlots = 0;
    while ((1 << logSlots) < kDistSlotsPerOctave) ++logSlots;
    uint32_t distance = 5;
    while (distance <= kMaxDistance && base.size() < 256) {
        int width = 0;
        while ((uint64_t(1) << (width + 1)) <= distance) ++width;
        int extraBits = std::max(0, width - logSlots);
        base.push_back(distance);
        extra.push_back(static_cast<uint8_t>(extraBits));
        distance += (1u << extraBits);
    }
}

int DistanceModel::symbolFor(uint32_t distance) const {
    if (distance == 0 || distance > kMaxDistance) fail("distance out of range");
    auto it = std::upper_bound(base.begin(), base.end(), distance);
    return static_cast<int>(it - base.begin()) - 1;
}

const DistanceModel& distanceModel() {
    static const DistanceModel model;
    return model;
}

DistanceCode codeDistance(const DistanceModel& model, uint32_t distance,
                          RepState& reps) {
    for (int i = 0; i < kRepCount; ++i) {
        if (reps.distance[i] == distance) {
            for (int j = i; j > 0; --j) reps.distance[j] = reps.distance[j - 1];
            reps.distance[0] = distance;
            return DistanceCode{static_cast<uint16_t>(model.base.size() + i), 0, 0};
        }
    }
    int slot = model.symbolFor(distance);
    for (int j = kRepCount - 1; j > 0; --j) reps.distance[j] = reps.distance[j - 1];
    reps.distance[0] = distance;
    return DistanceCode{static_cast<uint16_t>(slot),
                        distance - model.base[static_cast<size_t>(slot)],
                        model.extra[static_cast<size_t>(slot)]};
}

namespace {

void appendNibbles(std::vector<uint8_t>& out, const std::vector<uint8_t>& lengths) {
    size_t i = 0;
    while (i < lengths.size()) {
        uint8_t low = lengths[i++] & 0x0F;
        uint8_t high = 0;
        if (i < lengths.size()) high = lengths[i++] & 0x0F;
        out.push_back(static_cast<uint8_t>(low | (high << 4)));
    }
}

std::vector<uint8_t> encodeLengthsNibbles(const std::vector<uint8_t>& lengths) {
    std::vector<uint8_t> out;
    appendNibbles(out, lengths);
    return out;
}

// Byte tokens: 0..15 literal length, 16 = zero run + varint, 17 = repeat the
// previous value + varint (additional repeats).  Single zeros stay literal.
std::vector<uint8_t> encodeLengthsRle(const std::vector<uint8_t>& lengths) {
    std::vector<uint8_t> out;
    size_t i = 0;
    while (i < lengths.size()) {
        uint8_t value = lengths[i];
        size_t run = 1;
        while (i + run < lengths.size() && lengths[i + run] == value) ++run;
        if (value == 0 && run >= 2) {
            out.push_back(16);
            appendVarint(out, run);
        } else {
            out.push_back(value);
            if (run >= 2) {
                out.push_back(17);
                appendVarint(out, run - 1);
            }
        }
        i += run;
    }
    return out;
}

// Huffman-codes the sequence of 16 possible length values.  The 16 code
// lengths of that tiny alphabet are stored as nibbles (8 bytes), followed by
// the bit-packed sequence and byte padding.
std::vector<uint8_t> encodeLengthsHuffman(const std::vector<uint8_t>& lengths) {
    std::array<uint32_t, 16> freq{};
    for (uint8_t v : lengths) {
        if (v > 15) fail("code length exceeds 15");
        ++freq[v];
    }
    std::vector<uint32_t> frequencies(freq.begin(), freq.end());
    HuffmanTable table = buildHuffman(frequencies, kMaxCodeLen);
    std::vector<uint8_t> out;
    appendNibbles(out, table.length);
    {
        BitWriter writer(out);
        for (uint8_t v : lengths) {
            writer.putCode(table.code[v], table.length[v]);
        }
        writer.align();
    }
    return out;
}

void decodeLengthsNibbles(const std::vector<uint8_t>& in, size_t& pos,
                          size_t count, std::vector<uint8_t>& out) {
    size_t bytes = (count + 1) / 2;
    if (in.size() - pos < bytes) fail("truncated code lengths");
    out.resize(count);
    for (size_t i = 0; i < count; ++i) {
        out[i] = static_cast<uint8_t>((in[pos + i / 2] >> ((i & 1) * 4)) & 0x0F);
    }
    pos += bytes;
}

void decodeLengthsRle(const std::vector<uint8_t>& in, size_t& pos,
                      size_t count, std::vector<uint8_t>& out) {
    out.clear();
    while (out.size() < count) {
        if (pos >= in.size()) fail("truncated RLE code lengths");
        uint8_t token = in[pos++];
        if (token <= 15) {
            out.push_back(token);
        } else if (token == 16) {
            uint64_t run = readVarint(in, pos);
            if (run == 0 || run > count - out.size()) fail("invalid zero run");
            out.insert(out.end(), static_cast<size_t>(run), 0);
        } else if (token == 17) {
            uint64_t run = readVarint(in, pos);
            if (out.empty() || run == 0 || run > count - out.size())
                fail("invalid repeat run");
            out.insert(out.end(), static_cast<size_t>(run), out.back());
        } else {
            fail("invalid RLE token");
        }
    }
}

void decodeLengthsHuffman(const std::vector<uint8_t>& in, size_t& pos,
                          size_t count, std::vector<uint8_t>& out) {
    if (in.size() - pos < 8) fail("truncated Huffman code lengths");
    std::vector<uint8_t> subLengths(16);
    for (size_t i = 0; i < 16; ++i) {
        subLengths[i] = static_cast<uint8_t>((in[pos + i / 2] >> ((i & 1) * 4)) & 0x0F);
    }
    pos += 8;
    HuffmanDecoder decoder;
    decoder.init(subLengths);
    BitReader reader(in.data() + pos, in.size() - pos);
    out.resize(count);
    for (size_t i = 0; i < count; ++i) out[i] = static_cast<uint8_t>(decoder.decode(reader));
    if (!reader.zeroPaddingToByteBoundary()) fail("nonzero metadata padding");
    pos += (reader.bitsUsed() + 7) / 8;
}

}  // namespace

EncodedLengths encodeLengths(const std::vector<uint8_t>& lengths) {
    EncodedLengths best;
    best.mode = kMetaNibbles;
    best.bytes = encodeLengthsNibbles(lengths);
    if (lengths.empty()) return best;

    std::vector<uint8_t> rle = encodeLengthsRle(lengths);
    if (rle.size() < best.bytes.size()) {
        best.mode = kMetaRle;
        best.bytes = std::move(rle);
    }
    std::vector<uint8_t> huffman = encodeLengthsHuffman(lengths);
    if (huffman.size() < best.bytes.size()) {
        best.mode = kMetaHuffman;
        best.bytes = std::move(huffman);
    }
    return best;
}

void decodeLengths(uint8_t mode, const std::vector<uint8_t>& in, size_t& pos,
                   size_t count, std::vector<uint8_t>& out) {
    switch (mode) {
        case kMetaNibbles: decodeLengthsNibbles(in, pos, count, out); break;
        case kMetaRle:     decodeLengthsRle(in, pos, count, out); break;
        case kMetaHuffman: decodeLengthsHuffman(in, pos, count, out); break;
        default: fail("invalid metadata mode");
    }
}

}  // namespace lzh
