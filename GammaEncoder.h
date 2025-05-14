#pragma once
#include <vector>
#include <cmath>
#include <bitset>
#include <sstream>
#include <cstdint>

class GammaEncoder {
public:
    static std::string encode(const std::vector<uint32_t>& numbers) {
        if (numbers.empty()) return "";
        
        std::string result;
        uint32_t prev = 0;
        
        for (uint32_t num : numbers) {
            uint32_t diff = num - prev;
            prev = num;
            
            if (diff == 0) continue; // No debería ocurrir con doc IDs únicos
            
            // Codificar gamma de la diferencia
            diff++; // Porque gamma no puede codificar 0
            
            std::string binary = std::bitset<32>(diff).to_string();
            size_t first_one = binary.find('1');
            if (first_one == std::string::npos) {
                continue; // No debería ocurrir
            }
            
            binary = binary.substr(first_one);
            std::string unary = std::string(binary.length() - 1, '0') + '1';
            std::string offset = binary.substr(1);
            
            result += unary + offset;
        }
        
        return result;
    }
    
    static std::vector<uint32_t> decode(const std::string& str) {
        std::vector<uint32_t> result;
        if (str.empty()) return result;
        
        size_t pos = 0;
        uint32_t prev = 0;
        
        while (pos < str.length()) {
            // Leer parte unaria (longitud)
            size_t len = 0;
            while (pos < str.length() && str[pos] == '0') {
                len++;
                pos++;
            }
            
            if (pos >= str.length() || str[pos] != '1') {
                break; // Error
            }
            
            pos++; // Saltar el '1'
            
            // Leer parte offset (bits restantes)
            if (pos + len > str.length()) {
                break; // Error
            }
            
            std::string offset = "1" + str.substr(pos, len);
            pos += len;
            
            uint32_t diff = std::bitset<32>(offset).to_ulong();
            diff--; // Deshacer el +1 de la codificación
            
            prev += diff;
            result.push_back(prev);
        }
        
        return result;
    }
};