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

    for (std::size_t kandidat = 0; kandidat < pos; ++kandidat) {
        std::size_t lengde = 0;

        while (
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

    Match match = finnBesteMatch(data, 3);

    std::cout << "Avstand: " << match.avstand << '\n';
    std::cout << "Lengde: " << match.lengde << '\n';
}