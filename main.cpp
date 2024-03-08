#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>

#include "compression.hpp"

auto main() -> int {
    using std::chrono::duration;
    using std::chrono::duration_cast;
    using std::chrono::high_resolution_clock;
    using std::chrono::milliseconds;

    const std::string inputFile = "input/repeated.txt";

    // Run RLE Compression
    runLength rle;
    auto      startRLE = high_resolution_clock::now();
    rle.encode(inputFile, "output/rleEncoded.txt");
    auto endRLE = high_resolution_clock::now();
    rle.decode("output/rleEncoded.txt", "output/rleDecoded.txt");
    auto endRLD = high_resolution_clock::now();

    // Run LZW Compression with bit width 16, to emit 2 chars
    LZW<16> lzw;
    lzw.encode(inputFile, "output/lzwEncoded.txt");
    auto endLZWE = high_resolution_clock::now();
    lzw.decode("output/lzwEncoded.txt", "output/lzwDecoded.txt");
    auto endLZWD = high_resolution_clock::now();

    // Calculate Function Times
    auto RLE  = duration_cast<milliseconds>(endRLE - startRLE);
    auto RLD  = duration_cast<milliseconds>(endRLD - endRLE);
    auto LZWE = duration_cast<milliseconds>(endLZWE - endRLD);
    auto LZWD = duration_cast<milliseconds>(endLZWD - endLZWE);

    // Calculate compression amounts
    std::size_t RLESize  = std::filesystem::file_size("output/rleEncoded.txt");
    std::size_t RLDSize  = std::filesystem::file_size("output/rleDecoded.txt");
    std::size_t LZWESize = std::filesystem::file_size("output/lzwEncoded.txt");
    std::size_t LZWDSize = std::filesystem::file_size("output/lzwDecoded.txt");

    // Print statistics
    std::cout << "Run length encode took: " << RLE.count() << "ms\n";
    std::cout << "Run length decode took: " << RLD.count() << "ms\n";
    std::cout << "Run length compression ratio: " << static_cast<float>(RLESize) / RLDSize << "\n";

    std::cout << "LZW encode took: " << LZWE.count() << "ms\n";
    std::cout << "LZW decode took: " << LZWD.count() << "ms\n";
    std::cout << "LZW compression ratio: " << static_cast<float>(LZWESize) / LZWDSize << "\n";

    return 0;
}