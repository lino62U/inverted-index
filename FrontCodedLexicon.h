#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <cstdint>

struct CompressedEntry {
    uint8_t prefix_len;
    std::string suffix;
};

class FrontCodedLexicon {
public:
    FrontCodedLexicon(size_t block_size = 4) : block_size(block_size) {}

    void add_term(const std::string& term) {
        terms.push_back(term);
    }

    void compress() {
        std::sort(terms.begin(), terms.end());

        for (size_t i = 0; i < terms.size(); i += block_size) {
            std::vector<CompressedEntry> block;
            block.push_back({0, terms[i]});
            for (size_t j = i + 1; j < std::min(i + block_size, terms.size()); ++j) {
                int pl = common_prefix(terms[j - 1], terms[j]);
                block.push_back({static_cast<uint8_t>(pl), terms[j].substr(pl)});
            }
            blocks.push_back(block);
        }
    }

    void save_to_file(const std::string& filename, std::vector<uint64_t>& term_offsets) const {
        std::ofstream out(filename, std::ios::binary);
        uint64_t offset = 0;

        for (const auto& block : blocks) {
            for (const auto& entry : block) {
                term_offsets.push_back(offset);

                uint16_t len = entry.suffix.size();
                out.write(reinterpret_cast<const char*>(&entry.prefix_len), sizeof(uint8_t));
                out.write(reinterpret_cast<const char*>(&len), sizeof(uint16_t));
                out.write(entry.suffix.data(), len);

                offset += sizeof(uint8_t) + sizeof(uint16_t) + len;
            }
        }

        out.close();
    }

private:
    std::vector<std::string> terms;
    std::vector<std::vector<CompressedEntry>> blocks;
    size_t block_size;

    int common_prefix(const std::string& a, const std::string& b) {
        int len = 0;
        while (len < a.size() && len < b.size() && a[len] == b[len]) ++len;
        return len;
    }
};
