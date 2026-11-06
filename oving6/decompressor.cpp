#include "decompressor.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#include "bit_io.h"
#include "format.h"
#include "huffman.h"

namespace lzh {

std::vector<uint8_t> decompressData(const std::vector<uint8_t>& in) {
    if (in.size() < 5) fail("file too short");
    if (std::memcmp(in.data(), kMagic, 4) != 0) fail("bad magic (not an LZH1 file)");
    uint8_t mode = in[4];
    size_t pos = 5;
    uint64_t originalSize = readVarint(in, pos);
    if (originalSize > kMaxOutput) fail("declared output too large");

    if (mode == kModeStored) {
        if (in.size() - pos != originalSize) fail("stored payload size mismatch");
        return std::vector<uint8_t>(in.begin() + static_cast<std::ptrdiff_t>(pos), in.end());
    }
    if (mode != kModeBlocks) fail("unknown mode");

    const LengthModel& lm = lengthModel();
    const DistanceModel& dm = distanceModel();

    std::vector<uint8_t> out;
    out.reserve(static_cast<size_t>(originalSize));

    while (out.size() < originalSize) {
        uint64_t blockRaw = readVarint(in, pos);
        if (blockRaw == 0 || blockRaw > kMaxBlockRaw) fail("invalid block size");
        if (blockRaw > originalSize - out.size()) fail("block exceeds declared output size");

        uint64_t litCount = readVarint(in, pos);
        uint64_t distCount = readVarint(in, pos);
        if (litCount < 1 || litCount > 256 + lm.base.size())
            fail("invalid literal/length table size");
        if (distCount > dm.base.size() + kRepCount) fail("invalid distance table size");

        // One flags byte selects the representation of each code-length
        // table; reserved bits are zero.
        std::vector<uint8_t> litLengths;
        std::vector<uint8_t> distLengths;
        if (pos >= in.size()) fail("missing metadata flags");
        uint8_t flags = in[pos++];
        if ((flags & 0xF0) != 0) fail("reserved metadata flags set");
        uint8_t litMode = flags & 0x03;
        uint8_t distMode = static_cast<uint8_t>((flags >> 2) & 0x03);
        if (litMode > kMetaHuffman || distMode > kMetaHuffman)
            fail("invalid metadata mode");
        if (distCount == 0 && distMode != kMetaNibbles)
            fail("metadata mode for empty distance table");
        decodeLengths(litMode, in, pos, static_cast<size_t>(litCount), litLengths);
        decodeLengths(distMode, in, pos, static_cast<size_t>(distCount), distLengths);

        uint64_t payloadSize = readVarint(in, pos);
        if (payloadSize > in.size() - pos) fail("truncated block payload");
        const uint8_t* payload = in.data() + pos;
        pos += static_cast<size_t>(payloadSize);

        HuffmanDecoder litDecoder;
        litDecoder.init(litLengths);
        HuffmanDecoder distDecoder;
        if (distCount > 0) distDecoder.init(distLengths);

        BitReader reader(payload, static_cast<size_t>(payloadSize));
        RepState reps;
        size_t blockStart = out.size();
        while (out.size() - blockStart < blockRaw) {
            int symbol = litDecoder.decode(reader);
            if (symbol < 0 || symbol >= static_cast<int>(litCount)) fail("symbol outside table");
            if (symbol < 256) {
                if (out.size() - blockStart >= blockRaw) fail("block overrun");
                out.push_back(static_cast<uint8_t>(symbol));
            } else {
                int lenSym = symbol - 256;
                if (lenSym >= static_cast<int>(lm.base.size())) fail("invalid length symbol");
                uint32_t length = lm.base[static_cast<size_t>(lenSym)] +
                                  reader.readBits(lm.extra[static_cast<size_t>(lenSym)]);
                if (length < kMinMatch || length > kMaxMatch) fail("invalid match length");
                if (out.size() - blockStart + length > blockRaw) fail("match crosses block end");
                if (distCount == 0) fail("match without distance table");
                int distSym = distDecoder.decode(reader);
                if (distSym < 0 || distSym >= static_cast<int>(distCount)) fail("distance symbol outside table");
                uint32_t distance;
                if (distSym >= static_cast<int>(dm.base.size())) {
                    int index = distSym - static_cast<int>(dm.base.size());
                    if (index >= kRepCount) fail("invalid repeat-distance symbol");
                    distance = reps.distance[index];
                    if (distance == 0) fail("repeat distance before any match");
                    for (int j = index; j > 0; --j) reps.distance[j] = reps.distance[j - 1];
                    reps.distance[0] = distance;
                } else {
                    distance = dm.base[static_cast<size_t>(distSym)] +
                               reader.readBits(dm.extra[static_cast<size_t>(distSym)]);
                    for (int j = kRepCount - 1; j > 0; --j) reps.distance[j] = reps.distance[j - 1];
                    reps.distance[0] = distance;
                }
                if (distance == 0 || distance > out.size()) fail("invalid match distance");
                for (uint32_t i = 0; i < length; ++i) {
                    out.push_back(out[out.size() - distance]);
                }
            }
        }
        if (!reader.onlyZeroPadding()) fail("nonzero padding bits");
    }

    if (pos != in.size()) fail("trailing bytes after compressed data");
    if (out.size() != originalSize) fail("output size mismatch");
    return out;
}

}  // namespace lzh
