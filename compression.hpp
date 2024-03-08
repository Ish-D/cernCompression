#pragma once

#ifndef COMPRESSION_HPP
#define COMPRESSION_HPP

#include <bitset>
#include <fstream>
#include <queue>
#include <sstream>
#include <string>
#include <unordered_map>

class runLength {
public:
    void encode(const std::string& fileIn, const std::string& fileOut);
    void decode(const std::string& fileIn, const std::string& fileOut);
};

template <uint32_t BIT_WIDTH> class LZW {
public:
    void encode(const std::string& fileIn, const std::string& fileOut);
    void decode(const std::string& fileIn, const std::string& fileOut);
};

/* Run length encoding, but only encode lengths for sequences of two or more characters, denoted by that character twice.
   For example: WWWWWWWWWWWWBWWWWWWWWWWWWBBBWWWWWWWWWWWWWWWWWWWWWWWWBWWWWWWWWWWWWWW ->  WW12BWW12BB3WW24BWW14            */
void runLength::encode(const std::string& fileIn, const std::string& fileOut) {
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

/* Run length encoding, but only encode lengths for sequences of two or more characters, denoted by that character twice.
   For example: WW12BWW12BB3WW24BWW14 -> WWWWWWWWWWWWBWWWWWWWWWWWWBBBWWWWWWWWWWWWWWWWWWWWWWWWBWWWWWWWWWWWWWW*/
void runLength::decode(const std::string& fileIn, const std::string& fileOut) {
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

template <uint32_t BIT_WIDTH> void LZW<BIT_WIDTH>::encode(const std::string& fileIn, const std::string& fileOut) {
    const uint32_t dictSize = 1 << (BIT_WIDTH - 1);

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
            // Add current string to dictionary
            if (nextCode <= dictSize) {
                dict[cur] = nextCode++;
            }

            cur.erase(cur.size() - 1);

            // Output dictionary index as bit_width bits, or (bit_width/8) chars
            // Adjusted based off of bit_width, but heavily impacts encoded file size
            auto bits = std::bitset<BIT_WIDTH>(dict[cur]).to_string();
            for (int i = 0; i < BIT_WIDTH; i += 8) {
                output << static_cast<char>(std::bitset<8>(bits.substr(i, i + 8)).to_ulong());
            }

            cur = c;
        }
    }

    // Put remaining value in file
    if (cur.size()) {
        auto bits = std::bitset<BIT_WIDTH>(dict[cur]).to_string();
        for (int i = 0; i < BIT_WIDTH; i += 8) {
            output << static_cast<char>(std::bitset<8>(bits.substr(i, i + 8)).to_ulong());
        }
    }
}

template <uint32_t BIT_WIDTH> void LZW<BIT_WIDTH>::decode(const std::string& fileIn, const std::string& fileOut) {
    const uint32_t dictSize = 1 << (BIT_WIDTH - 1);

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

    for (uint64_t i = 0; i < input.size(); i += BIT_WIDTH / 8) {
        // Retrieve dictionary index from the two chars in file
        // E.g. 'AB' -> 01000001 01000010 -> 16706
        auto        substr = input.substr(i, BIT_WIDTH / 8);
        std::string bit_str;
        for (int i = 0; i < BIT_WIDTH / 8; i++) {
            bit_str += std::bitset<8>(substr[i]).to_string();
        }
        curCode = std::bitset<BIT_WIDTH>(bit_str).to_ullong();

        // If code not in dict, concat previous string and add to dict
        if (!dict.contains(curCode)) {
            dict[curCode] = prev + prev.substr(0, 1);
        }

        // Output current dictionary value
        output << dict[curCode];

        // Concatenate previous string to first symbol of output
        if (prev.size()) {
            dict[nextCode++] = prev + dict[curCode][0];
        }
        prev = dict[curCode];
    }
}

#endif