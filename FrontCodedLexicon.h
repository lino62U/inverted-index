#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <cstdint>
#include <unordered_map>


// Clase para manejar la compresión Front Coding del léxico
class FrontCodeLexicon {
private:
    std::string concatenated_terms;
    std::unordered_map<std::string, size_t> term_to_offset;
    size_t block_size;
    
    std::string compress_term(const std::string& term, const std::string& last_term) {
        if (last_term.empty()) return term;
        
        size_t common_prefix = 0;
        while (common_prefix < term.size() && 
               common_prefix < last_term.size() && 
               term[common_prefix] == last_term[common_prefix]) {
            common_prefix++;
        }
        
        return std::to_string(common_prefix) + term.substr(common_prefix);
    }

public:
    FrontCodeLexicon(size_t block_size = 100) : block_size(block_size) {}
    
    void add_terms(const std::vector<std::string>& terms) {
        if (terms.empty()) return;
        
        std::string block_data;
        std::string last_term_in_block;
        
        for (const auto& term : terms) {
            if (term_to_offset.find(term) != term_to_offset.end()) {
                continue; // Término ya existe
            }
            
            std::string compressed = compress_term(term, last_term_in_block);
            term_to_offset[term] = concatenated_terms.size() + block_data.size();
            block_data += compressed + '\0'; // Separador nulo
            
            last_term_in_block = term;
            
            if (term_to_offset.size() % block_size == 0) {
                concatenated_terms += block_data;
                block_data.clear();
                last_term_in_block.clear();
            }
        }
        
        if (!block_data.empty()) {
            concatenated_terms += block_data;
        }
    }
    
    size_t get_term_offset(const std::string& term) const {
        auto it = term_to_offset.find(term);
        if (it != term_to_offset.end()) {
            return it->second;
        }
        return static_cast<size_t>(-1);
    }
    
    std::string get_term_at_offset(size_t offset) const {
        if (offset >= concatenated_terms.size()) return "";
        
        size_t block_start = (offset / block_size) * block_size;
        if (block_start >= concatenated_terms.size()) block_start = 0;
        
        std::string term;
        size_t current_offset = block_start;
        
        while (current_offset <= offset) {
            if (current_offset >= concatenated_terms.size()) break;
            
            size_t term_end = concatenated_terms.find('\0', current_offset);
            if (term_end == std::string::npos) break;
            
            std::string compressed = concatenated_terms.substr(current_offset, term_end - current_offset);
            
            if (current_offset == block_start) {
                term = compressed;
            } else {
                size_t prefix_len = 0;
                size_t i = 0;
                while (i < compressed.size() && isdigit(compressed[i])) {
                    prefix_len = prefix_len * 10 + (compressed[i] - '0');
                    i++;
                }
                
                if (prefix_len > term.size()) {
                    return "";
                }
                
                term = term.substr(0, prefix_len) + compressed.substr(i);
            }
            
            current_offset = term_end + 1;
        }
        
        return term;
    }
    
    void save_to_file(const std::string& filename) const {
        std::ofstream out(filename, std::ios::binary);
        out.write(reinterpret_cast<const char*>(&block_size), sizeof(block_size));
        
        size_t concat_size = concatenated_terms.size();
        out.write(reinterpret_cast<const char*>(&concat_size), sizeof(concat_size));
        out.write(concatenated_terms.data(), concat_size);
        
        size_t map_size = term_to_offset.size();
        out.write(reinterpret_cast<const char*>(&map_size), sizeof(map_size));
        
        for (const auto& entry : term_to_offset) {
            size_t term_size = entry.first.size();
            out.write(reinterpret_cast<const char*>(&term_size), sizeof(term_size));
            out.write(entry.first.data(), term_size);
            
            out.write(reinterpret_cast<const char*>(&entry.second), sizeof(entry.second));
        }
    }
    
    void load_from_file(const std::string& filename) {
        std::ifstream in(filename, std::ios::binary);
        if (!in) return;
        
        in.read(reinterpret_cast<char*>(&block_size), sizeof(block_size));
        
        size_t concat_size;
        in.read(reinterpret_cast<char*>(&concat_size), sizeof(concat_size));
        concatenated_terms.resize(concat_size);
        in.read(&concatenated_terms[0], concat_size);
        
        size_t map_size;
        in.read(reinterpret_cast<char*>(&map_size), sizeof(map_size));
        
        term_to_offset.clear();
        for (size_t i = 0; i < map_size; ++i) {
            size_t term_size;
            in.read(reinterpret_cast<char*>(&term_size), sizeof(term_size));
            
            std::string term(term_size, '\0');
            in.read(&term[0], term_size);
            
            size_t offset;
            in.read(reinterpret_cast<char*>(&offset), sizeof(offset));
            
            term_to_offset[term] = offset;
        }
    }
};