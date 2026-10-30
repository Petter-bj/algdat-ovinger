#include <cstddef>
#include <vector>
#include <iostream>

struct Match {
    std::size_t avstand;
    std::size_t lengde;
};

Match finnBesteMatch(
    const std::vector<unsigned char>& data,
    std::size_t pos
) {
    Match beste{0, 0};

    // Let maks 255 bytes
    std::size_t start = (pos > 255) ? pos - 255 : 0;

    for (std::size_t kandidat = start; kandidat < pos; ++kandidat) {
        std::size_t lengde = 0;

        while (
            lengde < 255 &&
            pos + lengde < data.size() &&
            kandidat + lengde < pos &&
            data[kandidat + lengde] == data[pos + lengde]
        ) {
            ++lengde;
        }

        if (lengde > beste.lengde) {
            beste.avstand = pos - kandidat;
            beste.lengde = lengde;
        }
    }

    return beste;
}

int main() {
    std::vector<unsigned char> data{
        'A', 'B', 'C', 'A', 'B', 'C', 'A', 'B', 'C', '!'
    };

    std::vector<unsigned char> komprimert;
    std::size_t pos = 0;

    while (pos < data.size()) {
        Match match = finnBesteMatch(data, pos);

        if (match.lengde >= 3) {
            // Referanse: merke, avstand, lengde
            komprimert.push_back(1);
            komprimert.push_back(
                static_cast<unsigned char>(match.avstand)
                );
            komprimert.push_back(
                static_cast<unsigned char>(match.lengde)
                );

            pos += match.lengde;
        } else {
            komprimert.push_back(0);
            komprimert.push_back(data[pos]);

            ++pos;
        }
    }
    for (unsigned char byte : komprimert) {
        std::cout << static_cast<unsigned int>(byte) << ' ';
    }

    std::cout << "\n";
    std::cout << "Orginal:" << data.size() << "bytes \n";
    std::cout << "Komprimert " << komprimert.size() << "byets\n";
}