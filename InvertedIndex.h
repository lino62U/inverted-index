#pragma once
#include "FrontCodedLexicon.h"
#include "GammaEncoder.h"

#include <unordered_map>
#include <vector>
#include <string>
#include <fstream>
#include <set>
#include <iostream>
#include <cassert>


class InvertedIndex {
private:
    FrontCodeLexicon lexicon;
    std::string concatenated_doc_ids;
    std::unordered_map<size_t, size_t> term_offset_to_doclist_offset;
    
    void update_doclist(size_t term_offset, const std::vector<uint32_t>& new_docs) {
        std::vector<uint32_t> existing_docs;
        
        // Obtener docs existentes si hay
        auto it = term_offset_to_doclist_offset.find(term_offset);
        if (it != term_offset_to_doclist_offset.end()) {
            std::string encoded = concatenated_doc_ids.substr(it->second);
            existing_docs = GammaEncoder::decode(encoded);
        }
        
        // Fusionar y ordenar
        std::vector<uint32_t> merged_docs(existing_docs);
        merged_docs.insert(merged_docs.end(), new_docs.begin(), new_docs.end());
        std::sort(merged_docs.begin(), merged_docs.end());
        merged_docs.erase(std::unique(merged_docs.begin(), merged_docs.end()), merged_docs.end());
        
        // Codificar la nueva lista
        std::string encoded = GammaEncoder::encode(merged_docs);
        
        // Actualizar o agregar
        if (it != term_offset_to_doclist_offset.end()) {
            // Intentar reutilizar el espacio si es suficiente
            size_t old_offset = it->second;
            std::string old_encoded = concatenated_doc_ids.substr(old_offset);
            if (encoded.size() <= old_encoded.size()) {
                for (size_t i = 0; i < encoded.size(); ++i) {
                    concatenated_doc_ids[old_offset + i] = encoded[i];
                }
                return;
            }
        }
        
        // Agregar al final
        term_offset_to_doclist_offset[term_offset] = concatenated_doc_ids.size();
        concatenated_doc_ids += encoded;
    }
    
public:
    InvertedIndex() : lexicon(100) {}
    
    void add_document(uint32_t doc_id, const std::vector<std::string>& terms) {
        std::unordered_map<std::string, std::vector<uint32_t>> term_doc_map;
        
        for (const auto& term : terms) {
            term_doc_map[term].push_back(doc_id);
        }

        // Imprimir el term_doc_map
        std::cout << "Term -> Documents:\n";
        for (const auto& entry : term_doc_map) {
            const std::string& term = entry.first;
            const std::vector<uint32_t>& doc_list = entry.second;
            
            std::cout << "Term: " << term << " -> Docs: ";
            for (uint32_t doc : doc_list) {
                std::cout << doc << " ";
            }
            std::cout << std::endl;
        }
        
        for (const auto& entry : term_doc_map) {
            const std::string& term = entry.first;
            size_t term_offset = lexicon.get_term_offset(term);
            
            if (term_offset == static_cast<size_t>(-1)) {
                lexicon.add_terms({term});
                term_offset = lexicon.get_term_offset(term);
            }
            
            update_doclist(term_offset, entry.second);
        }
    }
    
    std::vector<uint32_t> search(const std::string& term) const {
        size_t term_offset = lexicon.get_term_offset(term);
        if (term_offset == static_cast<size_t>(-1)) {
            return {};
        }
        
        auto it = term_offset_to_doclist_offset.find(term_offset);
        if (it == term_offset_to_doclist_offset.end()) {
            return {};
        }
        
        std::string encoded = concatenated_doc_ids.substr(it->second);
        return GammaEncoder::decode(encoded);
    }
    
    void save_to_files(const std::string& lexicon_file, const std::string& docids_file) const {
        lexicon.save_to_file(lexicon_file);
        
        std::ofstream out(docids_file, std::ios::binary);
        
        size_t concat_size = concatenated_doc_ids.size();
        out.write(reinterpret_cast<const char*>(&concat_size), sizeof(concat_size));
        out.write(concatenated_doc_ids.data(), concat_size);
        
        size_t map_size = term_offset_to_doclist_offset.size();
        out.write(reinterpret_cast<const char*>(&map_size), sizeof(map_size));
        
        for (const auto& entry : term_offset_to_doclist_offset) {
            out.write(reinterpret_cast<const char*>(&entry.first), sizeof(entry.first));
            out.write(reinterpret_cast<const char*>(&entry.second), sizeof(entry.second));
        }
    }
    
    void load_from_files(const std::string& lexicon_file, const std::string& docids_file) {
        lexicon.load_from_file(lexicon_file);
        
        std::ifstream in(docids_file, std::ios::binary);
        if (!in) return;
        
        size_t concat_size;
        in.read(reinterpret_cast<char*>(&concat_size), sizeof(concat_size));
        concatenated_doc_ids.resize(concat_size);
        in.read(&concatenated_doc_ids[0], concat_size);
        
        size_t map_size;
        in.read(reinterpret_cast<char*>(&map_size), sizeof(map_size));
        
        term_offset_to_doclist_offset.clear();
        for (size_t i = 0; i < map_size; ++i) {
            size_t term_offset, doclist_offset;
            in.read(reinterpret_cast<char*>(&term_offset), sizeof(term_offset));
            in.read(reinterpret_cast<char*>(&doclist_offset), sizeof(doclist_offset));
            
            term_offset_to_doclist_offset[term_offset] = doclist_offset;
        }
    }
};
