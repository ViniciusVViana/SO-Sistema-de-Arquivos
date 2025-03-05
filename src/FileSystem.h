#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <cmath>

#include <cstdint>
#include "DiskManager.h"


//Criar a função min
template <typename T>
T getMin(T a, T b){
    return a < b ? a : b;
}

class FileSystem {
private:
    DiskManager diskManager;
    Superblock superblock;

    //Calcular numero de blocos para o bitmap
    u_int32_t calcNumBlocksBitmap(u_int32_t numBlocks){
        return (numBlocks + BLOCK_SIZE * 8 - 1) / (BLOCK_SIZE * 8);
    }


public:
    FileSystem(string &path) : diskManager(path){}

    //Inicializar o disco com o superbloco e bitmap
    void init(u_int32_t numBlocks){
        diskManager.open();
        
        superblock.total_blocks =  numBlocks;
        superblock.bitmap_blocks = calcNumBlocksBitmap(numBlocks);
        superblock.root_dir_index = 1 + superblock.bitmap_blocks;
        superblock.free_blocks = numBlocks - 2 - superblock.bitmap_blocks;

        //Inicializar o superbloco no bloco 0
        diskManager.writeBlock(0, (char*)&superblock);

        //Inicializar o bitmap
        vector<uint8_t> bitmap(BLOCK_SIZE * superblock.bitmap_blocks, 0);
        for (u_int32_t i = 0; i < superblock.bitmap_start + superblock.bitmap_blocks; i++){
            bitmap[i / 8] |= 1 << (i % 8);

        }

        diskManager.writeBlock(1, (char*)bitmap.data());

        diskManager.close();

    }

    // Alocar um bloco
    u_int32_t allocBlock(){
        diskManager.open();

        vector<uint8_t> bitmap(BLOCK_SIZE * superblock.bitmap_blocks);
        diskManager.readBlock(superblock.bitmap_start, (char*)bitmap.data());

        for (u_int32_t i = 0; i < superblock.total_blocks; i++){
            if (!(bitmap[i / 8] & (1 << (i % 8)))){
                bitmap[i / 8] |= 1 << (i % 8);
                superblock.free_blocks--;
                diskManager.writeBlock(superblock.bitmap_start, (char*)bitmap.data());
                diskManager.writeBlock(0, (char*)&superblock);
                diskManager.close();
                return i;
            }
        }

        diskManager.close();
        return 0xFFFFFFFF;
    }

    //Liberar um bloco
    void freeBlock(u_int32_t blockIndex){
        if(blockIndex >= superblock.total_blocks){
            throw std::runtime_error("Bloco Inválido!");
        }
        diskManager.open();

        vector<uint8_t> bitmap(BLOCK_SIZE * superblock.bitmap_blocks);
        diskManager.readBlock(superblock.bitmap_start, (char*)bitmap.data());

        bitmap[blockIndex / 8] &= ~(1 << (blockIndex % 8));
        superblock.free_blocks++;
        diskManager.writeBlock(superblock.bitmap_start, (char*)bitmap.data());
        diskManager.writeBlock(0, (char*)&superblock);
        diskManager.close();
    }

    // Criar um arquivo no diretório raiz
    void createFile(string &filename, char filetype){
        diskManager.open();
        uint32_t index_block = allocBlock();
        if(index_block == 0xFFFFFFFF){
            throw std::runtime_error("Não há blocos disponíveis!");
        }

        RootDirEntry entry;
        strncpy(entry.filename, filename.c_str(), FILENAME_SIZE);
        entry.filename[FILENAME_SIZE] = '\0';
        entry.file_type = filetype;
        entry.index_block = index_block;

        //Escrever no dir raiz
        vector<RootDirEntry> rootDir(BLOCK_SIZE / sizeof(RootDirEntry));
        diskManager.readBlock(superblock.root_dir_index, (char*)rootDir.data());

        for(auto &entry : rootDir){
            if(entry.filename[0] == '\0'){
                entry = entry;
                diskManager.writeBlock(superblock.root_dir_index, (char*)rootDir.data());
                diskManager.close();
                return;
            }
        }

        diskManager.close();
        throw std::runtime_error("Diretório raiz cheio!");
    }

    //Ler um arquivo do diretório raiz
    void readFile(string &filename, char *filetype, u_int32_t *index_block){
        diskManager.open();
        vector<RootDirEntry> rootDir(BLOCK_SIZE / ENTRY_SIZE);
        diskManager.readBlock(superblock.root_dir_index, (char*)rootDir.data());

        for(auto &entry : rootDir){
            if(strcmp(entry.filename, filename.c_str()) == 0){
                *filetype = entry.file_type;
                *index_block = entry.index_block;
                diskManager.close();
                return;
            }
        }

        diskManager.close();
        throw std::runtime_error("Arquivo não encontrado!");
    }

    //Excluir um arquivo do diretório raiz
    void deleteFile(string &filename){
        diskManager.open();
        vector<RootDirEntry> rootDir(BLOCK_SIZE / ENTRY_SIZE);
        diskManager.readBlock(superblock.root_dir_index, (char*)rootDir.data());

        for(auto &entry : rootDir){
            if(strcmp(entry.filename, filename.c_str()) == 0){
                IndexBlock ib;
                diskManager.readBlock(entry.index_block, (char*)&ib);
                for(auto ptr : ib.block_ptrs){
                    if(ptr != 0xFFFFFFFF){
                        freeBlock(ptr);
                    }
                }
                return;
            }
        }

        diskManager.close();
        throw std::runtime_error("Arquivo não encontrado!");
    }

    //Escrever em um arquivo
    void writeFile(uint32_t index_block, uint32_t block_offset, const char *data, uint32_t size) {
        diskManager.open();
    
        IndexBlock ib;
        diskManager.readBlock(index_block, reinterpret_cast<char*>(&ib));
    
        uint32_t current_block_offset = block_offset;
    
        // Função auxiliar para escrever dados em um bloco de índice
        auto writeToIndexBlock = [&](IndexBlock &ib, uint32_t &current_block_offset, const char *&data, uint32_t &size) {
            for (uint32_t i = current_block_offset; i < ib.block_ptrs.size(); i++) {
                if (size == 0) {
                    break;
                }
    
                // Se o ponteiro direto estiver vazio, aloca um novo bloco
                if (ib.block_ptrs[i] == 0xFFFFFFFF) {
                    ib.block_ptrs[i] = allocBlock();
                }
    
                // Escreve os dados no bloco de dados
                uint32_t write_size = getMin<uint32_t>(size, BLOCK_SIZE);
                diskManager.writeBlock(ib.block_ptrs[i], (char *)data);
                size -= write_size;
                data += write_size;
                current_block_offset++;
            }
        };
    
        // Escreve nos ponteiros diretos do bloco de índice atual
        writeToIndexBlock(ib, current_block_offset, data, size);
    
        // Se ainda houver dados para escrever, usa ponteiros indiretos
        while (size > 0) {
            // Se não houver um bloco de índice indireto, aloca um
            if (ib.indirect_ptr == 0xFFFFFFFF) {
                ib.indirect_ptr = allocBlock();
            }
    
            // Lê o bloco de índice indireto
            IndexBlock indirect_ib;
            diskManager.readBlock(ib.indirect_ptr, reinterpret_cast<char*>(&indirect_ib));
    
            // Escreve nos ponteiros diretos do bloco de índice indireto
            writeToIndexBlock(indirect_ib, current_block_offset, data, size);
    
            // Atualiza o bloco de índice indireto no disco
            diskManager.writeBlock(ib.indirect_ptr, reinterpret_cast<char*>(&indirect_ib));
        }
    
        // Atualiza o bloco de índice no disco
        diskManager.writeBlock(index_block, reinterpret_cast<char*>(&ib));
    
        diskManager.close();
    }

    //Ler de um arquivo
    void readFile(uint32_t index_block, uint32_t block_offset, char *data, uint32_t size) {
        diskManager.open();
    
        IndexBlock ib;
        diskManager.readBlock(index_block, reinterpret_cast<char*>(&ib));
    
        uint32_t current_block_offset = block_offset;
    
        // Função auxiliar para ler dados de um bloco de índice
        auto readFromIndexBlock = [&](IndexBlock &ib, uint32_t &current_block_offset, char *&data, uint32_t &size) {
            for (uint32_t i = current_block_offset; i < ib.block_ptrs.size(); i++) {
                if (size == 0) {
                    break;
                }
    
                // Lê os dados do bloco de dados
                uint32_t read_size = getMin<uint32_t>(size, BLOCK_SIZE);
                diskManager.readBlock(ib.block_ptrs[i], data);
                size -= read_size;
                data += read_size;
                current_block_offset++;
            }
        };
    
        // Lê dos ponteiros diretos do bloco de índice atual
        readFromIndexBlock(ib, current_block_offset, data, size);
    
        // Se ainda houver dados para ler, usa ponteiros indiretos
        while (size > 0) {
            // Lê o bloco de índice indireto
            IndexBlock indirect_ib;
            diskManager.readBlock(ib.indirect_ptr, reinterpret_cast<char*>(&indirect_ib));
    
            // Lê dos ponteiros diretos do bloco de índice indireto
            readFromIndexBlock(indirect_ib, current_block_offset, data, size);
        }
    
        diskManager.close();
    }

    //Listar os arquivos do diretório raiz
    void listFiles() {
        diskManager.open();
    
        vector<RootDirEntry> rootDir(BLOCK_SIZE / ENTRY_SIZE);
        diskManager.readBlock(superblock.root_dir_index, reinterpret_cast<char*>(rootDir.data()));
    
        for (const auto &entry : rootDir) {
            if (entry.filename[0] != '\0') {
                cout << "Filename: " << entry.filename << ", Type: " << entry.file_type << ", Index Block: " << entry.index_block << endl;
            }
        }
    
        diskManager.close();
    }

    //Listar os blocos livres
    void listFreeBlocks() {
        diskManager.open();
    
        vector<uint8_t> bitmap(BLOCK_SIZE * superblock.bitmap_blocks);
        diskManager.readBlock(superblock.bitmap_start, reinterpret_cast<char*>(bitmap.data()));
    
        for (uint32_t i = 0; i < superblock.total_blocks; i++) {
            if (!(bitmap[i / 8] & (1 << (i % 8))) && i != 0) {
                cout << i << " ";
            }
        }
    
        cout << endl;
    
        diskManager.close();
    }

    //Listar o superbloco
    void listSuperblock() {
        diskManager.open();
    
        diskManager.readBlock(0, reinterpret_cast<char*>(&superblock));
    
        cout << "Total Blocks: " << superblock.total_blocks << endl;
        cout << "Bitmap Blocks: " << superblock.bitmap_blocks << endl;
        cout << "Root Directory Index: " << superblock.root_dir_index << endl;
        cout << "Free Blocks: " << superblock.free_blocks << endl;
    
        diskManager.close();
    }

    //Listar o bloco de índice
    void listIndexBlock(uint32_t index_block) {
        diskManager.open();
    
        IndexBlock ib;
        diskManager.readBlock(index_block, reinterpret_cast<char*>(&ib));
    
        cout << "Index Block: " << index_block << endl;
        cout << "Direct Pointers: ";
        for (const auto &ptr : ib.block_ptrs) {
            cout << ptr << " ";
        }
        cout << endl;
        cout << "Indirect Pointer: " << ib.indirect_ptr << endl;
    
        diskManager.close();
    }

    //Listar um bloco de dados
    void listDataBlock(uint32_t data_block) {
        diskManager.open();
    
        char data[BLOCK_SIZE];
        diskManager.readBlock(data_block, data);
    
        cout << "Data Block: " << data_block << endl;
        cout << "Data: " << data << endl;
    
        diskManager.close();
    }

    // Listar mapa de bits
    void listBitmap() {
        diskManager.open();
    
        vector<uint8_t> bitmap(BLOCK_SIZE * superblock.bitmap_blocks);
        diskManager.readBlock(superblock.bitmap_start, reinterpret_cast<char*>(bitmap.data()));
    
        for (uint32_t i = 0; i < superblock.total_blocks; i++) {
            cout << ((bitmap[i / 8] & (1 << (i % 8))) ? "1" : "0");
        }
    
        cout << endl;
    
        diskManager.close();
    }

    //Listar o disco
    void listDisk() {
        diskManager.open();
    
        for (uint32_t i = 0; i < superblock.total_blocks; i++) {
            char data[BLOCK_SIZE];
            diskManager.readBlock(i, data);
    
            cout << "Block: " << i << endl;
            cout << "Data: " << data << endl;
        }
    
        diskManager.close();
    }

    //Listar o disco em hexadecimal
    void listDiskHex() {
        diskManager.open();
    
        for (uint32_t i = 0; i < superblock.total_blocks; i++) {
            char data[BLOCK_SIZE];
            diskManager.readBlock(i, data);
    
            cout << "Block: " << i << endl;
            cout << "Data: ";
            for (uint32_t j = 0; j < BLOCK_SIZE; j++) {
                cout << hex << (uint32_t)data[j] << " ";
            }
            cout << endl;
        }
    
        diskManager.close();
    }

    
};