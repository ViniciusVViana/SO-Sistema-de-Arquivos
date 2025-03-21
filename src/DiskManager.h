#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <cmath>
#include <cstdint>
#include "estruturas.h"

using namespace std;

class DiskManager {
private:
    fstream disk;
    string path;

public:
    DiskManager(string path) : path(path) {}

    void open() {
        disk.open(path, ios::in | ios::out | ios::binary);
        if (!disk.is_open()) {
            throw std::runtime_error("Erro ao abrir o disco");
        }
    }

    void close() {
        if (disk.is_open()) {
            disk.close();
        } else {
            cout << "O gerenciador de disco já estava fechado." << endl;
        }
    }

    void create(int size) {
        disk.open(path, ios::out | ios::binary);
        if (!disk.is_open()) {
            throw std::runtime_error("Erro ao criar o disco");
        }
        disk.seekp(size - 1);
        if (!disk) {
            throw std::runtime_error("Erro ao posicionar o ponteiro de escrita ao criar o disco");
        }
        disk.write("", 1);
        if (!disk) {
            throw std::runtime_error("Erro ao escrever no disco ao criar o disco");
        }
        disk.close();
    }

    void writeBlock(uint32_t block_number, void* data) {
        if (!disk.is_open()) {
            disk.open(path, std::ios::in | std::ios::out | std::ios::binary);
            if (!disk.is_open()) {
                throw std::runtime_error("Erro ao abrir o disco");
            }
        }
        disk.seekp(block_number * BLOCK_SIZE, std::ios::beg);
        if (!disk) {
            throw std::runtime_error("Erro ao posicionar o ponteiro de escrita");
        }
        disk.write(reinterpret_cast<const char*>(data), BLOCK_SIZE);
        if (!disk) {
            throw std::runtime_error("Erro ao escrever no disco");
        }
    }

    void readBlock(uint32_t block_number, void* data) {
        if (!disk.is_open()) {
            disk.open(path, std::ios::in | std::ios::out | std::ios::binary);
            if (!disk.is_open()) {
                throw std::runtime_error("Erro ao abrir o disco");
            }
        }

        // Verificar o tamanho do arquivo
        disk.clear(); // Limpar qualquer estado de erro anterior
        disk.seekg(0, std::ios::end);
        if (!disk) {
            throw std::runtime_error("Erro ao posicionar o ponteiro no final do arquivo");
        }

        std::uint64_t fileSize = disk.tellg();
        if (fileSize == static_cast<std::uint64_t>(-1)) {
            throw std::runtime_error("Erro ao obter o tamanho do arquivo");
        }

        std::uint64_t blockPosition = static_cast<std::uint64_t>(block_number) * BLOCK_SIZE;
        if (blockPosition >= fileSize) {
            throw std::runtime_error("Bloco fora dos limites do arquivo");
        }

        // Posicionar o ponteiro de leitura
        disk.seekg(blockPosition, std::ios::beg);
        if (!disk) {
            throw std::runtime_error("Erro ao posicionar o ponteiro de leitura");
        }

        // Ler os dados
        disk.read(reinterpret_cast<char*>(data), BLOCK_SIZE);
        if (!disk) {
            throw std::runtime_error("Erro ao ler do disco");
        }
    }

    std::streampos getFileSize() {
        if (!disk.is_open()) {
            disk.open(path, std::ios::in | std::ios::out | std::ios::binary);
            if (!disk.is_open()) {
                throw std::runtime_error("Erro ao abrir o disco");
            }
        }
        disk.seekg(0, std::ios::end);
        return disk.tellg();
    }

    ~DiskManager() {
        close();
    }
};