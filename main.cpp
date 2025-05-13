#include "InvertedIndex.h"

int main() {
    InvertedIndex idx;
    idx.add_document(1, "la casa roja");
    idx.add_document(2, "el perro de la casa");
    idx.add_document(3, "la casa blanca");

    idx.build_and_save(
        "lexicon.bin",
        "term_ptrs.bin",
        "postings.bin",
        "post_ptrs.bin"
    );

    std::cout << "Índice invertido guardado exitosamente.\n";
}
