#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "error.h"

namespace lzh {

// Container and framing constants.
constexpr size_t kBlockSize    = size_t(1) << 21;  // 2 MiB per block
constexpr size_t kMinMatch     = 3;
constexpr size_t kMaxMatch     = 4098;
constexpr uint32_t kMaxDistance = 1u << 22;  // distance alphabet covers 4 MiB
constexpr uint32_t kMaxBlockRaw = 64u << 20; // decoder sanity limit
constexpr uint64_t kMaxOutput   = 8ull << 30; // decoder sanity limit

extern const uint8_t kMagic[4];
constexpr uint8_t kModeStored = 0;
constexpr uint8_t kModeBlocks = 2;  // metadata-mode flags (mode 1 obsolete)

// Code-length table representations (two bits per table in the flags byte).
constexpr uint8_t kMetaNibbles = 0;
constexpr uint8_t kMetaRle     = 1;
constexpr uint8_t kMetaHuffman = 2;

void appendVarint(std::vector<uint8_t>& out, uint64_t value);

// Reads an unsigned LEB128 value.  At most 10 bytes are accepted; the tenth
// byte may only contribute the single remaining bit and must terminate.
// Non-minimal encodings are accepted; oversized and overlong ones are not.
uint64_t readVarint(const std::vector<uint8_t>& in, size_t& pos);

// ---------------------------------------------------------------------------
// Length and distance alphabets.
// ---------------------------------------------------------------------------

// Length symbols start with 29 codes for lengths 3..258 (small lengths are
// direct, larger ones get extra bits, two symbols per width; symbol 28 is
// exactly 258).  Above 258 the alphabet continues, four symbols per octave,
// so long repeats need fewer tokens.
struct LengthModel {
    std::vector<uint16_t> base;
    std::vector<uint8_t> extra;
    std::vector<uint8_t> symbol;      // indexed by length
    std::vector<uint16_t> extraValue; // indexed by length

    LengthModel();
};

const LengthModel& lengthModel();

// Distance symbols cover 1..2^22.  The first four distances are direct; above
// that each octave is split into two slots.  More slots mean fewer raw extra
// bits per match at the cost of a larger Huffman alphabet; two slots per
// octave measured best on the official files.
struct DistanceModel {
    std::vector<uint32_t> base;
    std::vector<uint8_t> extra;

    DistanceModel();
    int symbolFor(uint32_t distance) const;
};

const DistanceModel& distanceModel();

constexpr int kRepCount = 8;  // repeat-previous-distance symbols per block

// Codes one match distance.  If the distance equals one of the last eight
// distances it costs a single Huffman symbol and no extra bits; otherwise the
// geometric slot is used.  Both encoder and decoder run this on the same token
// sequence, so the state stays in sync without being stored.
struct RepState {
    uint32_t distance[kRepCount] = {0, 0, 0, 0, 0, 0, 0, 0};
};

struct DistanceCode {
    uint16_t symbol;
    uint32_t extraValue;
    uint8_t extraBits;
};

DistanceCode codeDistance(const DistanceModel& model, uint32_t distance,
                          RepState& reps);

// ---------------------------------------------------------------------------
// Code-length metadata.
// ---------------------------------------------------------------------------

struct EncodedLengths {
    uint8_t mode = kMetaNibbles;
    std::vector<uint8_t> bytes;
};

// Picks the smallest of the generic representations (nibbles, RLE, Huffman).
EncodedLengths encodeLengths(const std::vector<uint8_t>& lengths);

// Decodes exactly count lengths using the given representation.
void decodeLengths(uint8_t mode, const std::vector<uint8_t>& in, size_t& pos,
                   size_t count, std::vector<uint8_t>& out);

}  // namespace lzh
