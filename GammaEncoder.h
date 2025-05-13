#pragma once
#include <vector>
#include <cmath>
#include <bitset>
#include <sstream>
#include <cstdint>

class GammaEncoder {
public:
    static std::string encode_number(uint32_t num) {
        if (num == 0) throw std::invalid_argument("gamma encoding undefined for 0");

        uint32_t n = log2(num);
        std::string unary(n, '1');
        unary += '0';

        std::bitset<32> binary(num);
        std::string offset = binary.to_string().substr(32 - n);

        return unary + offset;
    }

    static std::string encode_list(const std::vector<uint32_t>& numbers) {
        std::string bits;
        for (auto n : numbers) {
            bits += encode_number(n);
        }
        return bits;
    }

    static std::vector<uint8_t> to_bytes(const std::string& bitstring) {
        std::vector<uint8_t> bytes;
        for (size_t i = 0; i < bitstring.size(); i += 8) {
            std::bitset<8> b(bitstring.substr(i, std::min(8UL, bitstring.size() - i)));
            bytes.push_back(static_cast<uint8_t>(b.to_ulong()));
        }
        return bytes;
    }
};
