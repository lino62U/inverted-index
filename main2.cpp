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

namespace fs = std::filesystem;
using namespace std;
using namespace chrono;

// Estructura para manejar el índice por hilos
struct ThreadIndex {
    unordered_map<string, unordered_set<int>> localIndex;
    mutex mtx;

    void addWord(const string& word, int docID) {
        lock_guard<mutex> lock(mtx);
        localIndex[word].insert(docID);
    }
};

string normalizeWord(const string &word) {
    string normalized;
    locale loc;

    normalized.reserve(word.size());
    for (auto &ch : word) {
        if (isalpha(ch, loc)) {
            normalized += tolower(ch, loc);
        }
    }
    return normalized;
}

void processChunk(const char* start, const char* end, int docID, ThreadIndex& threadIndex) {
    const char* p = start;
    const char* wordStart = p;
    string word;
    
    while (p != end) {
        if (isspace(*p)) {
            if (wordStart != p) {
                word.assign(wordStart, p);
                string normalized = normalizeWord(word);
                if (!normalized.empty()) {
                    threadIndex.addWord(normalized, docID);
                }
            }
            wordStart = p + 1;
        }
        ++p;
    }
    
    // Procesar la última palabra si existe
    if (wordStart != p) {
        word.assign(wordStart, p);
        string normalized = normalizeWord(word);
        if (!normalized.empty()) {
            threadIndex.addWord(normalized, docID);
        }
    }
}

void processFile(const string& filename, int docID, ThreadIndex& threadIndex, size_t chunkSize = 64 * 1024 * 1024) {
    ifstream file(filename, ios::binary | ios::ate);
    if (!file.is_open()) {
        cerr << "Error al abrir archivo: " << filename << endl;
        return;
    }

    const size_t fileSize = file.tellg();
    file.seekg(0, ios::beg);

    vector<char> buffer(chunkSize);
    size_t remaining = fileSize;
    size_t overlapSize = 256; // Suficiente para la palabra más larga

    while (remaining > 0) {
        const size_t readSize = min(chunkSize, remaining);
        file.read(buffer.data(), readSize);

        // Encontrar el último espacio en blanco para dividir palabras correctamente
        size_t validSize = readSize;
        if (remaining > readSize) {
            validSize = 0;
            for (size_t i = readSize - 1; i > 0; --i) {
                if (isspace(buffer[i])) {
                    validSize = i + 1;
                    break;
                }
            }
            if (validSize == 0) validSize = readSize; // No se encontró espacio
        }

        processChunk(buffer.data(), buffer.data() + validSize, docID, threadIndex);

        // Preparar para la siguiente lectura
        if (remaining > readSize) {
            // Mover el resto al inicio del buffer para la próxima lectura
            size_t overlap = readSize - validSize;
            memmove(buffer.data(), buffer.data() + validSize, overlap);
            file.seekg(-static_cast<streamoff>(overlap), ios::cur);
            remaining += overlap; // Ajustar el remaining
        }

        remaining -= validSize;
    }
}

void mergeIndexes(vector<unique_ptr<ThreadIndex>>& threadIndexes, unordered_map<string, unordered_set<int>>& globalIndex) {
    // Primero recolectar todas las palabras únicas
    unordered_set<string> allWords;
    for (const auto& ti : threadIndexes) {
        for (const auto& entry : ti->localIndex) {
            allWords.insert(entry.first);
        }
    }

    // Convertir a vector para procesamiento paralelo
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

void saveIndexToTxt(const unordered_map<string, unordered_set<int>>& index, const string& filename) {
    ofstream outFile(filename);
    if (!outFile.is_open()) {
        cerr << "No se pudo abrir el archivo para guardar el índice.\n";
        return;
    }

    // Convertir a vector para ordenar solo por palabra
    vector<pair<string, unordered_set<int>>> sortedIndex(index.begin(), index.end());
    sort(sortedIndex.begin(), sortedIndex.end(), 
        [](const auto& a, const auto& b) { return a.first < b.first; });

    for (const auto& entry : sortedIndex) {
        outFile << entry.first << ":";
        // Convertir docIDs a vector para ordenarlos
        vector<int> sortedDocs(entry.second.begin(), entry.second.end());
        sort(sortedDocs.begin(), sortedDocs.end());
        for (int docID : sortedDocs) {
            outFile << " " << docID;
        }
        outFile << "\n";
    }
}

int main() {
    auto start = high_resolution_clock::now(); // Tiempo de inicio

    // Configuración automática de hilos
    int numThreads = omp_get_max_threads();
    cout << "Usando " << numThreads << " hilos disponibles\n";
    omp_set_num_threads(numThreads);

    vector<string> files = {"file1.txt", "file2.txt", "file3.txt", "file4.txt"};
    
    // Crear índices por hilo para evitar contención
    vector<unique_ptr<ThreadIndex>> threadIndexes(numThreads);
    for (auto& ti : threadIndexes) {
        ti = make_unique<ThreadIndex>();
    }

    // Procesar archivos en paralelo
    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < files.size(); ++i) {
        int threadNum = omp_get_thread_num();
        processFile(files[i], i, *threadIndexes[threadNum]);
    }

    // Fusionar índices
    unordered_map<string, unordered_set<int>> globalIndex;
    mergeIndexes(threadIndexes, globalIndex);

    // Guardar en archivo de texto
    saveIndexToTxt(globalIndex, "inverted_index.txt");

    // Medir el tiempo total
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    cout << "Índice invertido construido y guardado exitosamente.\n";
    cout << "Tiempo total de procesamiento: " << duration.count() << " ms" << endl;

    return 0;
}
