#include <bitset>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <queue>
#include <sstream>
#include <string>
#include <unordered_map>

/* Run length encoding, but only encode lengths for sequences of two or more characters, denoted by that character twice.
   For example: WWWWWWWWWWWWBWWWWWWWWWWWWBBBWWWWWWWWWWWWWWWWWWWWWWWWBWWWWWWWWWWWWWW ->  WW12BWW12BB3WW24BWW14            */
void runLengthEncode(const std::string& fileIn, const std::string& fileOut) {
    // Read entire file into input string
    std::ifstream     file(fileIn);
    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string input = buffer.str();

    std::ofstream output(fileOut, std::ios::out | std::ios::binary);
    uint32_t      runLen;

    for (size_t i = 0; i < input.size(); i++) {
        // Reset current run length
        runLen = 1;

        // Count number of consecutive characters
        while (input[i] == input[i + 1]) {
            runLen++;
            i++;
        }

        // Append length & character to output if it appears twice or more
        if (runLen == 1) {
            output << input[i];
        } else {
            output << std::string(2, input[i]);
            output << std::to_string(runLen);
        }
    }
}

void runLengthDecode(const std::string& fileIn, const std::string& fileOut) {
    // Read entire file into input string
    std::ifstream     file(fileIn);
    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string input = buffer.str();

    std::ofstream output(fileOut, std::ios::out | std::ios::binary);
    int           i = 0;

    while (i < input.size()) {
        // Check if we have escape sequence of same character twice
        if (input[i] == input[i + 1]) {
            char cur  = input[i];
            i        += 2;

            // Isolate the digits in the string
            int runLen = 0;
            while (isdigit(input[i])) {
                runLen *= 10;
                runLen += input[i] - '0';
                i++;
            }

            // Add run length number of current character to output
            output << std::string(runLen, cur);
        } else {
            // Add lone character to output
            output << input[i];
            i++;
        }
    }
}

constexpr uint32_t bits     = 16;
constexpr int      dictSize = 1 << (bits - 1);

void lzwEncode(const std::string& fileIn, const std::string& fileOut) {
    // Read entire file into input string
    std::ifstream     file(fileIn);
    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string input = buffer.str();

    std::ofstream output(fileOut, std::ios::out | std::ios::binary);

    // Initialize dictionary with ASCII values
    std::unordered_map<std::string, uint32_t> dict(dictSize);
    for (uint32_t i = 0; i < 256; i++) {
        dict[std::string(1, i)] = i;
    }

    std::string cur;
    uint32_t    curCode  = 0;
    uint32_t    nextCode = 256;
    uint32_t    maxidx   = 0;

    for (const char c : input) {
        cur += c;
        if (!dict.contains(cur)) {
            if (nextCode <= dictSize) {
                dict[cur] = nextCode++;
            }

            cur.erase(cur.size() - 1);

            // Store dictionary index as 16 bits, or 2 chars
            // Value needs to be adjusted based off the bit width being used
            auto bits = std::bitset<16>(dict[cur]).to_string();
            output << static_cast<char>(std::bitset<8>(bits.substr(0, 8)).to_ulong())
                   << static_cast<char>(std::bitset<8>(bits.substr(8, 16)).to_ulong());
            cur = c;
        }
    }
    // Put remaining value in file
    if (cur.size()) {
        auto bits = std::bitset<16>(dict[cur]).to_string();
        output << static_cast<char>(std::bitset<8>(bits.substr(0, 8)).to_ulong()) << static_cast<char>(std::bitset<8>(bits.substr(8, 16)).to_ulong());
    }
}

void lzwDecode(const std::string& fileIn, const std::string& fileOut) {
    // Read entire file into input string
    std::ifstream     file(fileIn);
    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string input = buffer.str();

    // Initialize dictionary with ASCII values of length 1
    std::unordered_map<uint32_t, std::string> dict(dictSize);
    for (uint32_t i = 0; i < 256; i++) {
        dict[i] = std::string(1, i);
    }

    std::string   prev;
    uint32_t      curCode  = 0;
    uint32_t      nextCode = 256;
    std::ofstream output(fileOut);

    for (uint64_t i = 0; i < input.size(); i += 2) {
        // Retrieve dictionary index from the two chars in file
        // E.g. 'AB' -> 01000001 01000010 -> 16706
        auto substr = input.substr(i, 2);
        curCode     = std::bitset<16>(std::bitset<8>(substr[0]).to_string() + std::bitset<8>(substr[1]).to_string()).to_ullong();

        if (!dict.contains(curCode)) {
            dict[curCode] = prev + prev.substr(0, 1);
        }
        output << dict[curCode];
        if (prev.size()) {
            dict[nextCode++] = prev + dict[curCode][0];
        }
        prev = dict[curCode];
    }
}

auto main() -> int {
    using std::chrono::duration;
    using std::chrono::duration_cast;
    using std::chrono::high_resolution_clock;
    using std::chrono::milliseconds;

    const std::string inputFile = "input/file.txt";

    auto startRLE = high_resolution_clock::now();
    runLengthEncode(inputFile, "output/rleEncoded");
    auto endRLE = high_resolution_clock::now();
    runLengthDecode("output/rleEncoded", "output/rleDecoded");
    auto endRLD = high_resolution_clock::now();

    lzwEncode(inputFile, "output/lzwEncoded");
    auto endLZWE = high_resolution_clock::now();
    lzwDecode("output/lzwEncoded", "output/lzwDecoded");
    auto endLZWD = high_resolution_clock::now();

    // Calculate Function Times
    auto RLE  = duration_cast<milliseconds>(endRLE - startRLE);
    auto RLD  = duration_cast<milliseconds>(endRLD - endRLE);
    auto LZWE = duration_cast<milliseconds>(endLZWE - endRLD);
    auto LZWD = duration_cast<milliseconds>(endLZWD - endLZWE);

    // Calculate compression amounts

    // Print statistics
    std::size_t RLESize   = std::filesystem::file_size("output/rleEncoded");
    std::size_t RLDSize   = std::filesystem::file_size("output/rleDecoded");
    std::size_t LZWESize  = std::filesystem::file_size("output/lzwEncoded");
    std::size_t LZWDSize  = std::filesystem::file_size("output/lzwDecoded");
    std::size_t inputSize = std::filesystem::file_size(inputFile);

    assert(inputSize == RLDSize);
    assert(inputSize == LZWDSize);

    std::cout << "Run length encode took: " << RLE.count() << "ms\n";
    std::cout << "Run length decode took: " << RLD.count() << "ms\n";
    std::cout << "Run length compression ratio: " << static_cast<float>(RLESize) / RLDSize << "\n";

    std::cout << "LZW encode took: " << LZWE.count() << "ms\n";
    std::cout << "LZW decode took: " << LZWD.count() << "ms\n";
    std::cout << "LZW compression ratio: " << static_cast<float>(LZWESize) / LZWDSize << "\n";

    return 0;
}