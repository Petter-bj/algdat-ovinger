#include "huffman.h"

#include <algorithm>
#include <queue>
#include <utility>

namespace lzh {

// Builds length-limited canonical Huffman code lengths.
//
// Standard Huffman depths are computed first.  Depths above maxLen are then
// repaired with the classic "move one leaf down, give the overflow leaf a
// sibling" correction, which keeps the code complete.  If the repair ever
// fails its Kraft check (should not happen), a complete uniform code is used
// as a safe fallback.
HuffmanTable buildHuffman(const std::vector<uint32_t>& freq, int maxLen) {
    const int n = static_cast<int>(freq.size());
    HuffmanTable table;
    table.length.assign(n, 0);
    table.code.assign(n, 0);

    std::vector<int> used;
    used.reserve(n);
    for (int s = 0; s < n; ++s) {
        if (freq[s] != 0) used.push_back(s);
    }
    if (used.empty()) return table;
    if (used.size() == 1) {
        table.length[used[0]] = 1;
        table.code[used[0]] = 0;
        return table;
    }

    struct Node {
        uint64_t weight;
        int symbol;
        int left;
        int right;
    };
    std::vector<Node> nodes;
    nodes.reserve(used.size() * 2);

    auto greater = [&nodes](int a, int b) {
        if (nodes[a].weight != nodes[b].weight)
            return nodes[a].weight > nodes[b].weight;
        return a > b;
    };
    std::priority_queue<int, std::vector<int>, decltype(greater)> queue(greater);

    for (int s : used) {
        nodes.push_back({freq[s], s, -1, -1});
        queue.push(static_cast<int>(nodes.size()) - 1);
    }
    while (queue.size() > 1) {
        int a = queue.top();
        queue.pop();
        int b = queue.top();
        queue.pop();
        nodes.push_back({nodes[a].weight + nodes[b].weight, -1, a, b});
        queue.push(static_cast<int>(nodes.size()) - 1);
    }

    std::vector<int> depth(n, 0);
    std::vector<std::pair<int, int>> stack;
    stack.push_back({queue.top(), 0});
    while (!stack.empty()) {
        std::pair<int, int> entry = stack.back();
        stack.pop_back();
        int node = entry.first;
        int d = entry.second;
        if (nodes[node].symbol >= 0) {
            depth[nodes[node].symbol] = d;
        } else {
            stack.push_back({nodes[node].left, d + 1});
            stack.push_back({nodes[node].right, d + 1});
        }
    }

    std::vector<int> count(static_cast<size_t>(maxLen) + 1, 0);
    int overflow = 0;
    for (int s : used) {
        int d = depth[s];
        if (d > maxLen) {
            d = maxLen;
            ++overflow;
        }
        ++count[d];
    }
    while (overflow > 0) {
        int bits = maxLen - 1;
        while (bits > 0 && count[bits] == 0) --bits;
        if (bits == 0 || count[maxLen] == 0) {
            overflow = 0;
            break;
        }
        --count[bits];
        count[bits + 1] += 2;
        --count[maxLen];
        overflow -= 2;
    }

    // Verify that the lengths form a prefix code.  If not, fall back to a
    // uniform code (complete when the symbol count is a power of two,
    // otherwise still prefix-free).
    auto kraftOk = [&]() {
        uint64_t units = 0;
        for (int len = 1; len <= maxLen; ++len) {
            if (count[len] < 0) return false;
            units += static_cast<uint64_t>(count[len]) << (maxLen - len);
        }
        return units <= (uint64_t(1) << maxLen);
    };
    if (!kraftOk()) {
        int uniform = 1;
        while ((size_t(1) << uniform) < used.size()) ++uniform;
        if (uniform > maxLen) fail("huffman fallback length too large");
        std::fill(count.begin(), count.end(), 0);
        count[uniform] = static_cast<int>(used.size());
        if (!kraftOk()) fail("huffman fallback is not a prefix code");
    }

    // Longest codes go to the least frequent symbols.
    std::sort(used.begin(), used.end(), [&freq](int a, int b) {
        if (freq[a] != freq[b]) return freq[a] < freq[b];
        return a < b;
    });
    size_t next = 0;
    for (int len = maxLen; len >= 1; --len) {
        for (int k = 0; k < count[len]; ++k) {
            if (next >= used.size()) fail("huffman length assignment overflow");
            table.length[used[next++]] = static_cast<uint8_t>(len);
        }
    }
    if (next != used.size()) fail("huffman length assignment mismatch");

    // Canonical codes: symbols ordered by (length, symbol).
    std::vector<int> order = used;
    std::sort(order.begin(), order.end(), [&table](int a, int b) {
        if (table.length[a] != table.length[b])
            return table.length[a] < table.length[b];
        return a < b;
    });
    uint32_t code = 0;
    int previous = 0;
    for (int s : order) {
        int len = table.length[s];
        code <<= (len - previous);
        table.code[s] = code;
        ++code;
        previous = len;
    }
    return table;
}

void HuffmanDecoder::init(const std::vector<uint8_t>& lengths) {
    length_.assign(lengths.begin(), lengths.end());
    if (length_.empty()) fail("empty huffman table");
    int maxLen = 0;
    for (uint8_t l : length_) {
        if (l > kMaxCodeLen) fail("code length exceeds 15");
        maxLen = std::max<int>(maxLen, l);
    }
    maxLen_ = maxLen;
    if (maxLen_ == 0) fail("huffman table has no symbols");
    count_.assign(static_cast<size_t>(maxLen_) + 1, 0);
    for (uint8_t l : length_) {
        if (l != 0) ++count_[l];
    }
    firstCode_.assign(static_cast<size_t>(maxLen_) + 1, 0);
    firstIndex_.assign(static_cast<size_t>(maxLen_) + 1, 0);
    uint32_t code = 0;
    int index = 0;
    for (int len = 1; len <= maxLen_; ++len) {
        firstCode_[len] = code;
        firstIndex_[len] = index;
        if (static_cast<uint64_t>(code) + count_[len] > (uint64_t(1) << len)) {
            fail("over-subscribed huffman table");
        }
        code = (code + count_[len]) << 1;
        index += count_[len];
    }
    symbols_.assign(static_cast<size_t>(index), 0);
    int at = 0;
    for (int len = 1; len <= maxLen_; ++len) {
        for (size_t s = 0; s < length_.size(); ++s) {
            if (length_[s] == len) symbols_[at++] = static_cast<int>(s);
        }
    }
}

int HuffmanDecoder::decode(BitReader& reader) const {
    uint32_t code = 0;
    for (int len = 1; len <= maxLen_; ++len) {
        code = (code << 1) | reader.readBit();
        uint32_t first = firstCode_[len];
        if (code >= first) {
            uint32_t offset = code - first;
            if (offset < static_cast<uint32_t>(count_[len])) {
                return symbols_[static_cast<size_t>(firstIndex_[len]) + offset];
            }
        }
    }
    fail("invalid huffman code");
}

}  // namespace lzh
