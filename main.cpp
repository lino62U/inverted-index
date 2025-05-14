#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <cctype>
#include <vector>
#include <omp.h>
#include <mutex>
#include <locale>
#include <filesystem>
#include <algorithm>
#include <memory>
#include <cstring>
#include <chrono>
#include <stdexcept>

namespace fs = std::filesystem;
using namespace std;
using namespace chrono;

struct ThreadIndex {
    unordered_map<string, unordered_set<int>> localIndex;
    mutex mtx;

    void addWord(const string& word, int docID) {
        lock_guard<mutex> lock(mtx);
        localIndex[word].insert(docID);
    }
};

// Función para procesar el archivo con mmap
void processFileWithMmap(const string& filename, int docID, ThreadIndex& threadIndex) {
    try {
        int fd = open(filename.c_str(), O_RDONLY);
        if (fd == -1) {
            throw runtime_error("Error al abrir el archivo: " + filename);
        }

        struct stat sb;
        if (fstat(fd, &sb) == -1) {
            throw runtime_error("Error al obtener el tamaño del archivo: " + filename);
        }

        size_t fileSize = sb.st_size;
        char* fileContent = (char*)mmap(NULL, fileSize, PROT_READ, MAP_PRIVATE, fd, 0);
        if (fileContent == MAP_FAILED) {
            throw runtime_error("Error al mapear el archivo a memoria: " + filename);
        }

        const size_t chunkSize = 64 * 1024 * 1024; // 64 MB por bloque
        size_t remaining = fileSize;

        while (remaining > 0) {
            const size_t readSize = min(chunkSize, remaining);
            char* start = fileContent + (fileSize - remaining);
            char* end = start + readSize;

            if (end > fileContent + fileSize) {
                end = fileContent + fileSize;
            }

            processChunk(start, end, docID, threadIndex);
            remaining -= readSize;
        }

        if (munmap(fileContent, fileSize) == -1) {
            throw runtime_error("Error al desmapear el archivo.");
        }
        close(fd);
    }
    catch (const exception& e) {
        cerr << "Excepción: " << e.what() << endl;
    }
}

// Función para fusionar los índices con manejo de excepciones
void mergeIndexes(vector<unique_ptr<ThreadIndex>>& threadIndexes, unordered_map<string, unordered_set<int>>& globalIndex) {
    try {
        unordered_set<string> allWords;
        for (const auto& ti : threadIndexes) {
            if (!ti) {
                throw runtime_error("ThreadIndex no está inicializado correctamente.");
            }
            for (const auto& entry : ti->localIndex) {
                allWords.insert(entry.first);
            }
        }

        vector<string> words(allWords.begin(), allWords.end());
        #pragma omp parallel for schedule(dynamic)
        for (size_t i = 0; i < words.size(); ++i) {
            const string& word = words[i];
            unordered_set<int> docIDs;

            for (const auto& ti : threadIndexes) {
                auto it = ti->localIndex.find(word);
                if (it != ti->localIndex.end()) {
                    docIDs.insert(it->second.begin(), it->second.end());
                }
            }

            #pragma omp critical
            {
                globalIndex[word] = move(docIDs);
            }
        }
    }
    catch (const exception& e) {
        cerr << "Excepción al fusionar índices: " << e.what() << endl;
    }
}

int main() {
    try {
        auto start = high_resolution_clock::now();

        int numThreads = omp_get_max_threads();
        cout << "Usando " << numThreads << " hilos disponibles\n";
        omp_set_num_threads(numThreads);

        vector<string> files = {"file1.txt", "file2.txt", "file3.txt", "file4.txt"};
        vector<unique_ptr<ThreadIndex>> threadIndexes(numThreads);

        for (auto& ti : threadIndexes) {
            ti = make_unique<ThreadIndex>();
        }

        #pragma omp parallel for schedule(dynamic)
        for (int i = 0; i < files.size(); ++i) {
            int threadNum = omp_get_thread_num();
            processFileWithMmap(files[i], i, *threadIndexes[threadNum]);
        }

        unordered_map<string, unordered_set<int>> globalIndex;
        mergeIndexes(threadIndexes, globalIndex);

        saveIndexToTxt(globalIndex, "inverted_index.txt");

        auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start);
        cout << "Índice invertido construido y guardado exitosamente.\n";
        cout << "Tiempo total de procesamiento: " << duration.count() << " ms" << endl;
    }
    catch (const exception& e) {
        cerr << "Excepción en el programa principal: " << e.what() << endl;
    }

    return 0;
}
