#include <cstdint>
#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "compressor.h"
#include "decompressor.h"
#include "error.h"

namespace {

using lzh::fail;

std::vector<uint8_t> readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) fail("cannot open input: " + path);
    std::vector<uint8_t> data;
    file.seekg(0, std::ios::end);
    std::streamoff size = file.tellg();
    if (size < 0) fail("cannot determine size: " + path);
    file.seekg(0, std::ios::beg);
    data.resize(static_cast<size_t>(size));
    if (size > 0) {
        file.read(reinterpret_cast<char*>(data.data()), size);
        if (!file) fail("cannot read input: " + path);
    }
    return data;
}

void writeFile(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream file(path, std::ios::binary);
    if (!file) fail("cannot open output: " + path);
    if (!data.empty()) {
        file.write(reinterpret_cast<const char*>(data.data()),
                   static_cast<std::streamsize>(data.size()));
    }
    if (!file) fail("cannot write output: " + path);
}

void usage(const char* program) {
    std::cerr
        << "Usage: " << program << " <compress|decompress> <input> <output>\n"
        << "  compress     read input, write LZH1 compressed file\n"
        << "  decompress   read LZH1 file, write the original bytes\n";
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 4) {
        usage(argv[0]);
        return 1;
    }
    const std::string command = argv[1];
    const std::string inputPath = argv[2];
    const std::string outputPath = argv[3];
    try {
        if (command == "compress" || command == "c") {
            std::vector<uint8_t> input = readFile(inputPath);
            std::vector<uint8_t> output = lzh::compressData(input);
            writeFile(outputPath, output);
            std::cerr << "compressed " << input.size() << " -> " << output.size()
                      << " bytes\n";
        } else if (command == "decompress" || command == "d") {
            std::vector<uint8_t> input = readFile(inputPath);
            std::vector<uint8_t> output = lzh::decompressData(input);
            writeFile(outputPath, output);
            std::cerr << "decompressed " << input.size() << " -> " << output.size()
                      << " bytes\n";
        } else {
            usage(argv[0]);
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 2;
    }
    return 0;
}
