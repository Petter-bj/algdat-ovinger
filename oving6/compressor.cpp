#include "compressor.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <queue>
#include <utility>
#include <vector>

#include "bit_io.h"
#include "format.h"
#include "huffman.h"

namespace lzh {

namespace {

constexpr int kHashBits = 18;
constexpr int kMaxChain = 8192;  // hash-chain candidates per position
constexpr int kParseIterations = 3;  // greedy + two cost-driven parses

constexpr int kMatchBuckets = 7;
constexpr uint32_t kBucketDistance[kMatchBuckets] = {
    kMaxDistance, 1048576, 131072, 16384, 2048, 256, 32};

struct MatchSets {
    std::array<std::vector<uint16_t>, kMatchBuckets> length;
    std::array<std::vector<uint32_t>, kMatchBuckets> distance;
};

inline uint32_t hashAt(const uint8_t* data, size_t remaining, int bits) {
    // Three-byte hash: kMinMatch is 3, and a four-byte hash would hide exact
    // three-byte repeats whose fourth byte differs.
    (void)remaining;
    uint32_t value = static_cast<uint32_t>(data[0]) |
                     (static_cast<uint32_t>(data[1]) << 8) |
                     (static_cast<uint32_t>(data[2]) << 16);
    value *= 2654435761u;
    return value >> (32 - bits);
}

inline size_t matchLength(const uint8_t* data, size_t a, size_t b, size_t limit) {
    size_t len = 0;
    while (len + 8 <= limit) {
        uint64_t x, y;
        std::memcpy(&x, data + a + len, 8);
        std::memcpy(&y, data + b + len, 8);
        uint64_t diff = x ^ y;
        if (diff != 0) {
            size_t byte = 0;
            while ((diff & 0xFF) == 0) {
                diff >>= 8;
                ++byte;
            }
            return len + byte;
        }
        len += 8;
    }
    while (len < limit && data[a + len] == data[b + len]) ++len;
    return len;
}

void buildMatches(const uint8_t* data, size_t size, MatchSets& matches) {
    for (int b = 0; b < kMatchBuckets; ++b) {
        matches.length[b].assign(size, 0);
        matches.distance[b].assign(size, 0);
    }
    if (size < kMinMatch) return;

    const size_t headSize = size_t(1) << kHashBits;
    std::vector<int32_t> head(headSize, -1);
    std::vector<int32_t> previous(size, -1);

    for (size_t pos = 0; pos + kMinMatch <= size; ++pos) {
        const uint32_t h = hashAt(data + pos, size - pos, kHashBits);
        const size_t limit = std::min(kMaxMatch, size - pos);

        std::array<uint32_t, kMatchBuckets> bucketLen{};
        std::array<uint32_t, kMatchBuckets> bucketDist{};

        int32_t candidate = head[h];
        int chain = kMaxChain;
        while (candidate >= 0 && chain-- > 0) {
            const size_t distance = pos - static_cast<size_t>(candidate);
            if (distance > kMaxDistance) break;
            // Cheap rejection before the full compare.
            uint32_t longest = bucketLen[0];
            if (data[static_cast<size_t>(candidate)] == data[pos] &&
                (longest == 0 || data[static_cast<size_t>(candidate) + longest] == data[pos + longest])) {
                size_t len = matchLength(data, static_cast<size_t>(candidate), pos, limit);
                if (len >= kMinMatch) {
                    for (int b = 0; b < kMatchBuckets; ++b) {
                        if (distance > kBucketDistance[b]) continue;
                        if (len > bucketLen[b] ||
                            (len == bucketLen[b] && distance < bucketDist[b])) {
                            bucketLen[b] = static_cast<uint32_t>(len);
                            bucketDist[b] = static_cast<uint32_t>(distance);
                        }
                    }
                }
                if (bucketLen[0] >= limit) break;
            }
            candidate = previous[static_cast<size_t>(candidate)];
        }

        for (int b = 0; b < kMatchBuckets; ++b) {
            if (bucketLen[b] < kMinMatch) continue;
            bool duplicate = false;
            for (int earlier = 0; earlier < b && !duplicate; ++earlier) {
                duplicate = matches.length[earlier][pos] == bucketLen[b] &&
                            matches.distance[earlier][pos] == bucketDist[b];
            }
            if (!duplicate) {
                matches.length[b][pos] = static_cast<uint16_t>(bucketLen[b]);
                matches.distance[b][pos] = bucketDist[b];
            }
        }

        previous[pos] = head[h];
        head[h] = static_cast<int32_t>(pos);
    }
}

// ---------------------------------------------------------------------------
// Parsing.
// ---------------------------------------------------------------------------

struct Choice {
    uint16_t length = 0;   // 0 = literal
    uint32_t distance = 0;
};

struct ParseCosts {
    std::array<uint16_t, 256> literal{};
    std::array<uint16_t, kMaxMatch + 1> lengthCode{};
    std::vector<uint16_t> distanceCode;  // includes distance extra bits
    bool ready = false;
};

constexpr uint16_t kFallbackCost = 24;

ParseCosts costsFromTables(const HuffmanTable& litLen, const HuffmanTable& dist) {
    ParseCosts costs;
    const LengthModel& lm = lengthModel();
    const DistanceModel& dm = distanceModel();
    for (int b = 0; b < 256; ++b) {
        uint8_t len = b < static_cast<int>(litLen.length.size()) ? litLen.length[b] : 0;
        costs.literal[b] = len ? len : kFallbackCost;
    }
    for (size_t len = kMinMatch; len <= kMaxMatch; ++len) {
        int sym = 256 + lm.symbol[len];
        uint8_t bits = sym < static_cast<int>(litLen.length.size()) ? litLen.length[sym] : 0;
        costs.lengthCode[len] = static_cast<uint16_t>((bits ? bits : kFallbackCost) + lm.extra[lm.symbol[len]]);
    }
    costs.distanceCode.resize(dm.base.size());
    for (size_t slot = 0; slot < dm.base.size(); ++slot) {
        uint8_t bits = slot < dist.length.size() ? dist.length[slot] : 0;
        costs.distanceCode[slot] = static_cast<uint16_t>((bits ? bits : kFallbackCost) + dm.extra[slot]);
    }
    costs.ready = true;
    return costs;
}

// Greedy longest-match parse with one-step lazy evaluation.  Used only to get
// a first frequency distribution.
void parseGreedy(const uint8_t*, size_t size, const MatchSets& matches,
                 std::vector<Choice>& choices) {
    choices.assign(size, Choice{});
    size_t pos = 0;
    while (pos < size) {
        uint16_t length = matches.length[0][pos];
        uint32_t distance = matches.distance[0][pos];
        if (length < kMinMatch) {
            ++pos;
            continue;
        }
        if (pos + 1 < size && matches.length[0][pos + 1] > length) {
            ++pos;  // literal now, better match next
            continue;
        }
        choices[pos] = Choice{length, distance};
        pos += length;
    }
}

// Backwards dynamic program: at each position choose a literal or a match
// (with any length up to a stored candidate) that minimises total code cost.
void parseOptimal(const uint8_t* data, size_t size, const MatchSets& matches,
                  const ParseCosts& costs, std::vector<Choice>& choices) {
    choices.assign(size, Choice{});
    std::vector<int64_t> dp(size + 1, 0);
    const LengthModel& lm = lengthModel();
    const DistanceModel& dm = distanceModel();

    for (size_t pos = size; pos-- > 0;) {
        int64_t best = costs.literal[data[pos]] + dp[pos + 1];
        Choice bestChoice{};
        for (int candidate = 0; candidate < kMatchBuckets; ++candidate) {
            uint16_t limit = matches.length[candidate][pos];
            uint32_t distance = matches.distance[candidate][pos];
            if (limit < kMinMatch) continue;
            int slot = dm.symbolFor(distance);
            int64_t distanceCost = costs.distanceCode[static_cast<size_t>(slot)];
            uint16_t firstLen = kMinMatch;
            for (uint16_t len = firstLen; len <= limit; ++len) {
                int64_t value = distanceCost + costs.lengthCode[len] + dp[pos + len];
                if (value < best) {
                    best = value;
                    bestChoice = Choice{len, distance};
                }
            }
        }
        dp[pos] = best;
        choices[pos] = bestChoice;
    }
    (void)lm;
}

void gatherStats(const uint8_t* data, size_t size, const std::vector<Choice>& choices,
                 std::vector<uint32_t>& literalFreq, std::vector<uint32_t>& distanceFreq,
                 int& litLenSymbols, int& distanceSymbols) {
    const LengthModel& lm = lengthModel();
    const DistanceModel& dm = distanceModel();
    const int litLenAlphabet = 256 + static_cast<int>(lm.base.size());
    literalFreq.assign(static_cast<size_t>(litLenAlphabet), 0);
    distanceFreq.assign(dm.base.size() + kRepCount, 0);
    RepState reps;
    size_t pos = 0;
    while (pos < size) {
        Choice choice = choices[pos];
        if (choice.length == 0) {
            ++literalFreq[data[pos]];
            ++pos;
        } else {
            ++literalFreq[256 + lm.symbol[choice.length]];
            DistanceCode code = codeDistance(dm, choice.distance, reps);
            ++distanceFreq[code.symbol];
            pos += choice.length;
        }
    }
    litLenSymbols = litLenAlphabet;
    while (litLenSymbols > 1 && literalFreq[static_cast<size_t>(litLenSymbols - 1)] == 0)
        --litLenSymbols;
    distanceSymbols = static_cast<int>(distanceFreq.size());
    while (distanceSymbols > 0 && distanceFreq[static_cast<size_t>(distanceSymbols - 1)] == 0)
        --distanceSymbols;
}

// Serialises one complete block: raw size, tables, payload length, payload.
std::vector<uint8_t> encodeBlock(const uint8_t* data, size_t size,
                                 const std::vector<Choice>& choices,
                                 const HuffmanTable& litLen,
                                 const HuffmanTable& dist,
                                 int litLenSymbols, int distanceSymbols) {
    const LengthModel& lm = lengthModel();
    const DistanceModel& dm = distanceModel();

    std::vector<uint8_t> payload;
    {
        BitWriter writer(payload);
        RepState reps;
        size_t pos = 0;
        while (pos < size) {
            Choice choice = choices[pos];
            if (choice.length == 0) {
                uint8_t sym = data[pos];
                writer.putCode(litLen.code[sym], litLen.length[sym]);
                ++pos;
            } else {
                uint8_t lenSym = lm.symbol[choice.length];
                int symbol = 256 + lenSym;
                writer.putCode(litLen.code[static_cast<size_t>(symbol)],
                               litLen.length[static_cast<size_t>(symbol)]);
                writer.putBits(lm.extraValue[choice.length], lm.extra[lenSym]);
                DistanceCode code = codeDistance(dm, choice.distance, reps);
                writer.putCode(dist.code[code.symbol], dist.length[code.symbol]);
                writer.putBits(code.extraValue, code.extraBits);
                pos += choice.length;
            }
        }
        writer.align();
    }

    std::vector<uint8_t> out;
    appendVarint(out, size);
    appendVarint(out, static_cast<uint64_t>(litLenSymbols));
    appendVarint(out, static_cast<uint64_t>(distanceSymbols));

    std::vector<uint8_t> litLengths(litLen.length.begin(),
                                    litLen.length.begin() + litLenSymbols);
    EncodedLengths litMeta = encodeLengths(litLengths);
    EncodedLengths distMeta;
    if (distanceSymbols > 0) {
        std::vector<uint8_t> distLengths(dist.length.begin(),
                                         dist.length.begin() + distanceSymbols);
        distMeta = encodeLengths(distLengths);
    }
    uint8_t flags = static_cast<uint8_t>(litMeta.mode | (distMeta.mode << 2));
    out.push_back(flags);
    out.insert(out.end(), litMeta.bytes.begin(), litMeta.bytes.end());
    out.insert(out.end(), distMeta.bytes.begin(), distMeta.bytes.end());

    appendVarint(out, payload.size());
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

// Compresses one block.  A greedy parse provides the first statistics; each
// following iteration re-parses with the code costs of the previous tables and
// rebuilds the tables.  Every encoded candidate is kept if it is the smallest.
void compressBlock(const uint8_t* data, size_t size, std::vector<uint8_t>& out) {
    MatchSets matches;
    buildMatches(data, size, matches);

    std::vector<Choice> choices;
    parseGreedy(data, size, matches, choices);

    std::vector<uint32_t> litFreq, distFreq;
    int litSymbols = 0, distSymbols = 0;
    HuffmanTable litTable, distTable;
    std::vector<uint8_t> best;
    bool haveTables = false;

    for (int iteration = 0; iteration < kParseIterations; ++iteration) {
        if (haveTables) {
            ParseCosts costs = costsFromTables(litTable, distTable);
            parseOptimal(data, size, matches, costs, choices);
        }
        gatherStats(data, size, choices, litFreq, distFreq, litSymbols, distSymbols);
        std::vector<uint32_t> litTrim(litFreq.begin(), litFreq.begin() + litSymbols);
        std::vector<uint32_t> distTrim(distFreq.begin(), distFreq.begin() + distSymbols);
        litTable = buildHuffman(litTrim, kMaxCodeLen);
        distTable = buildHuffman(distTrim, kMaxCodeLen);
        haveTables = true;

        std::vector<uint8_t> candidate =
            encodeBlock(data, size, choices, litTable, distTable, litSymbols, distSymbols);
        if (best.empty() || candidate.size() < best.size()) {
            best = std::move(candidate);
        }
    }
    out.insert(out.end(), best.begin(), best.end());
}
}  // namespace

std::vector<uint8_t> compressData(const std::vector<uint8_t>& input) {
    std::vector<uint8_t> blocks;
    blocks.insert(blocks.end(), std::begin(kMagic), std::end(kMagic));
    blocks.push_back(kModeBlocks);
    appendVarint(blocks, input.size());
    for (size_t offset = 0; offset < input.size(); offset += kBlockSize) {
        size_t size = std::min(kBlockSize, input.size() - offset);
        compressBlock(input.data() + offset, size, blocks);
    }

    std::vector<uint8_t> stored;
    stored.insert(stored.end(), std::begin(kMagic), std::end(kMagic));
    stored.push_back(kModeStored);
    appendVarint(stored, input.size());
    stored.insert(stored.end(), input.begin(), input.end());

    return blocks.size() < stored.size() ? blocks : stored;
}
}  // namespace lzh
