#include "InvertedIndex.h"

int main() {
    InvertedIndex index;
    
    index.add_document(1, {"hello", "world", "test"});
    index.add_document(2, {"hello", "example"});
    index.add_document(3, {"world", "test", "example"});
    index.add_document(4, {"hello", "world", "example", "demo"});
    
    // Test de búsqueda
    auto test_search = [&](const std::string& term) {
        auto docs = index.search(term);
        std::cout << "Documentos con '" << term << "': ";
        for (uint32_t doc_id : docs) {
            std::cout << doc_id << " ";
        }
        std::cout << "\n";
    };
    
    test_search("hello");
    test_search("world");
    test_search("test");
    test_search("example");
    test_search("demo");
    test_search("nonexistent");
    
    return 0;
}