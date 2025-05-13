#pragma once
#include "FrontCodedLexicon.h"
#include "GammaEncoder.h"

#include <unordered_map>
#include <vector>
#include <string>
#include <fstream>
#include <set>
#include <iostream>

class InvertedIndex {
public:
    void add_document(int doc_id, const std::string& content) {
        std::istringstream iss(content);
        std::set<std::string> seen;
        std::string word;
        while (iss >> word) {
            if (seen.insert(word).second) {
                index[word].push_back(doc_id);
            }
        }
    }

    void build_and_save(const std::string& lexicon_file,
                        const std::string& term_ptr_file,
                        const std::string& postings_file,
                        const std::string& post_ptr_file) {
        // 1. Ordenar términos
        std::vector<std::string> sorted_terms;
        for (auto& [term, _] : index) sorted_terms.push_back(term);
        std::sort(sorted_terms.begin(), sorted_terms.end());

        // 2. Construir léxico
        FrontCodedLexicon lex(4);
        for (const auto& term : sorted_terms) lex.add_term(term);
        lex.compress();

        std::vector<uint64_t> lex_offsets;
        lex.save_to_file(lexicon_file, lex_offsets);

        // 3. Guardar punteros al léxico
        std::ofstream out_term_ptr(term_ptr_file, std::ios::binary);
        for (auto offset : lex_offsets) {
            out_term_ptr.write(reinterpret_cast<const char*>(&offset), sizeof(uint64_t));
        }
        out_term_ptr.close();

        // 4. Guardar postings
        std::ofstream out_postings(postings_file, std::ios::binary);
        std::ofstream out_post_ptr(post_ptr_file, std::ios::binary);
        uint64_t postings_offset = 0;

        for (const auto& term : sorted_terms) {
            const auto& doc_ids = index[term];
            std::vector<uint32_t> gaps;
            uint32_t prev = 0;
            for (auto id : doc_ids) {
                gaps.push_back(id - prev);
                prev = id;
            }

            std::string bits = GammaEncoder::encode_list(gaps);
            auto bytes = GammaEncoder::to_bytes(bits);

            out_post_ptr.write(reinterpret_cast<const char*>(&postings_offset), sizeof(uint64_t));
            out_postings.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());

            postings_offset += bytes.size();
        }

        out_postings.close();
        out_post_ptr.close();
    }

private:
    std::unordered_map<std::string, std::vector<uint32_t>> index;
};
