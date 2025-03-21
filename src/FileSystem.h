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
    vector<uint8_t> bitmap; 
    vector<RootDirEntry> rootDir;

    //Calcular numero de blocos para o bitmap
    u_int32_t calcNumBlocksBitmap(u_int32_t numBlocks){
        return (numBlocks + BLOCK_SIZE * 8 - 1) / (BLOCK_SIZE * 8);
    }


public:
    FileSystem(string &path, u_int32_t numBlocks) : diskManager(path) {
        if(numBlocks < 4){
            throw runtime_error("Número de blocos insuficiente!");
        }

        
        // Inicializa o vetor de bitmap
        bitmap.resize(BLOCK_SIZE * calcNumBlocksBitmap(numBlocks));
        // Inicializa o vetor de entradas de diretório raiz
        rootDir.resize(BLOCK_SIZE / ENTRY_SIZE);
        // Inicializa o superbloco
        superblock.total_blocks = numBlocks;
        superblock.bitmap_blocks = calcNumBlocksBitmap(numBlocks);
        superblock.root_dir_index = superblock.bitmap_blocks + 1;
        superblock.free_blocks = numBlocks - superblock.root_dir_index - 1;

        // Inicializa o superbloco no bloco 0
        diskManager.create(numBlocks * BLOCK_SIZE);
        diskManager.open();
        diskManager.writeBlock(0, (char*)&superblock);
        for (u_int32_t i = 0; i < superblock.bitmap_start + superblock.bitmap_blocks + 1; i++){
            bitmap[i / 8] |= 1 << (i % 8);
        }
        diskManager.writeBlock(superblock.bitmap_start, (char*)bitmap.data());
        diskManager.writeBlock(superblock.root_dir_index, (char*)rootDir.data());
        diskManager.writeBlock(1, (char*)bitmap.data());
        diskManager.close();
        
    }


    // Alocar um bloco
    u_int32_t allocBlock() {
        diskManager.open();
        diskManager.readBlock(superblock.bitmap_start, (char*)bitmap.data());
    
        for (u_int32_t i = 0; i < superblock.total_blocks; i++) { // Começa do bloco 1
            if (!(bitmap[i / 8] & (1 << (i % 8)))) { // Se o bloco estiver livre
                bitmap[i / 8] |= 1 << (i % 8); // Marcar o bloco como ocupado
                superblock.free_blocks--;
                
                // Atualiza o bitmap e o superbloco no disco
                diskManager.writeBlock(superblock.bitmap_start, (char*)bitmap.data());
                diskManager.writeBlock(0, (char*)&superblock);
                
                diskManager.close();
                return i; // Retorna o bloco alocado
            }
        }
    
        diskManager.close();
        return 0xFFFFFFFF; // Retorna erro se não houver blocos livres
    }

    //Liberar um bloco
    void freeBlock(u_int32_t blockIndex){
        if(blockIndex >= superblock.total_blocks){
            throw runtime_error("Bloco Inválido!");
        }
        diskManager.open();
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
            throw runtime_error("Não há blocos disponíveis!");
        }
        
        RootDirEntry newEntry;
        strncpy(newEntry.filename, filename.c_str(), FILENAME_SIZE);
        newEntry.filename[FILENAME_SIZE] = '\0';
        newEntry.file_type = filetype;
        newEntry.index_block = index_block;
        
        diskManager.readBlock(superblock.root_dir_index, (char*)rootDir.data());
        
        for(auto &entry : rootDir){
            if(entry.filename[0] == '\0'){
                entry = newEntry;
                diskManager.writeBlock(superblock.root_dir_index, (char*)rootDir.data());
                diskManager.close();
                cout<<"Arquivo criado com sucesso!"<<endl;
                return;
            }
        }
        
        diskManager.close();
        throw runtime_error("Diretório raiz cheio!");
    }

    //Ler um arquivo do diretório raiz
    void readFile(string &filename, char *filetype, u_int32_t *index_block){
        diskManager.open();
        
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
        throw runtime_error("Arquivo não encontrado!");
    }

    //Excluir um arquivo do diretório raiz
    void deleteFile(string &filename) {
        diskManager.open();
        
        diskManager.readBlock(superblock.root_dir_index, (char*)rootDir.data());
    
        for (auto &entry : rootDir) {
            if (strcmp(entry.filename, filename.c_str()) == 0) {
                IndexBlock ib;
                diskManager.readBlock(entry.index_block, (char*)&ib);
                
                // Libera os blocos de dados
                for (auto ptr : ib.block_ptrs) {
                    if (ptr != 0xFFFFFFFF) {
                        freeBlock(ptr);
                    }
                }
                
                // Libera o bloco de índice
                freeBlock(entry.index_block);
                
                // Remove a entrada do diretório raiz
                memset(entry.filename, 0, FILENAME_SIZE);
                entry.file_type = 0;
                entry.index_block = 0xFFFFFFFF;
                entry.file_size = 0;
                
                // Atualiza o diretório raiz no disco
                diskManager.writeBlock(superblock.root_dir_index, (char*)rootDir.data());
                
                diskManager.close();
                return;
            }
        }
    
        diskManager.close();
        throw runtime_error("Arquivo não encontrado!");
    }
    //Escrever em um arquivo
    void writeFile(const std::string &filename, const char *data, uint32_t size) {
        try {
            // Abre o gerenciador de disco
            diskManager.open();
    
            // Lê o bloco de índice do diretório raiz
            IndexBlock rootIndexBlock;
            diskManager.readBlock(superblock.root_dir_index, reinterpret_cast<char*>(&rootIndexBlock));
    
            // Procura o arquivo no diretório raiz
            bool fileFound = false;
            RootDirEntry fileEntry;
    
            for (uint32_t i = 0; i < rootIndexBlock.block_ptrs.size(); i++) {
                if (rootIndexBlock.block_ptrs[i] == 0xFFFFFFFF) {
                    continue; // Bloco de dados do diretório raiz não alocado
                }
    
                // Verifica se o ponteiro do bloco é válido
                if (rootIndexBlock.block_ptrs[i] >= superblock.total_blocks) {
                    throw std::runtime_error("Ponteiro de bloco de dados do diretório raiz inválido!");
                }
    
                // Lê o bloco de dados do diretório raiz
                std::vector<RootDirEntry> dirEntries(BLOCK_SIZE / sizeof(RootDirEntry));
                diskManager.readBlock(rootIndexBlock.block_ptrs[i], reinterpret_cast<char*>(dirEntries.data()));
    
                // Procura o arquivo no bloco de dados do diretório raiz
                for (const auto &entry : dirEntries) {
                    if (entry.filename[0] != '\0' && strcmp(entry.filename, filename.c_str()) == 0) {
                        fileFound = true;
                        fileEntry = entry;
                        break;
                    }
                }
    
                if (fileFound) {
                    break;
                }
            }
    
            if (!fileFound) {
                throw std::runtime_error("Arquivo não encontrado no diretório raiz!");
            }
    
            // Acessa o bloco de índice do arquivo
            IndexBlock fileIndexBlock;
            diskManager.readBlock(fileEntry.index_block, reinterpret_cast<char*>(&fileIndexBlock));
    
            // Escreve os dados no arquivo
            uint32_t bytesWritten = 0;
            uint32_t currentBlockOffset = 0;
    
            while (bytesWritten < size) {
                // Verifica se o offset está dentro dos limites
                if (currentBlockOffset >= fileIndexBlock.block_ptrs.size()) {
                    throw std::runtime_error("Offset de bloco fora dos limites!");
                }
    
                // Verifica se o ponteiro de bloco é válido
                if (fileIndexBlock.block_ptrs[currentBlockOffset] == 0xFFFFFFFF) {
                    // Aloca um novo bloco de dados
                    fileIndexBlock.block_ptrs[currentBlockOffset] = allocBlock();
                    if (fileIndexBlock.block_ptrs[currentBlockOffset] == 0xFFFFFFFF) {
                        throw std::runtime_error("Não há blocos disponíveis para alocação!");
                    }
                }
    
                if (fileIndexBlock.block_ptrs[currentBlockOffset] >= superblock.total_blocks) {
                    throw std::runtime_error("Ponteiro de bloco inválido!");
                }
    
                // Escreve os dados no bloco de dados
                uint32_t writeSize = std::min(size - bytesWritten, BLOCK_SIZE);
                diskManager.writeBlock(fileIndexBlock.block_ptrs[currentBlockOffset], reinterpret_cast<void*>(const_cast<char*>(data + bytesWritten)));
                bytesWritten += writeSize;
                currentBlockOffset++;
            }
    
            // Atualiza o bloco de índice do arquivo no disco
            diskManager.writeBlock(fileEntry.index_block, reinterpret_cast<char*>(&fileIndexBlock));
    
            // Atualiza o tamanho do arquivo no diretório raiz
            fileEntry.file_size = bytesWritten;
    
            // Atualiza a entrada do diretório raiz no disco
            for (uint32_t i = 0; i < rootIndexBlock.block_ptrs.size(); i++) {
                if (rootIndexBlock.block_ptrs[i] == 0xFFFFFFFF) {
                    continue; // Bloco de dados do diretório raiz não alocado
                }
    
                // Lê o bloco de dados do diretório raiz
                std::vector<RootDirEntry> dirEntries(BLOCK_SIZE / sizeof(RootDirEntry));
                diskManager.readBlock(rootIndexBlock.block_ptrs[i], reinterpret_cast<char*>(dirEntries.data()));
    
                // Procura o arquivo no bloco de dados do diretório raiz
                for (auto &entry : dirEntries) {
                    if (entry.filename[0] != '\0' && strcmp(entry.filename, filename.c_str()) == 0) {
                        entry = fileEntry;
                        diskManager.writeBlock(rootIndexBlock.block_ptrs[i], reinterpret_cast<char*>(dirEntries.data()));
                        break;
                    }
                }
            }
    
            // Fecha o gerenciador de disco
            diskManager.close();
    
            std::cout << "Dados escritos no arquivo com sucesso!" << std::endl;
        } catch (const std::exception &e) {
            // Fecha o gerenciador de disco em caso de erro
            diskManager.close();
            std::cerr << "Erro ao escrever no arquivo: " << e.what() << std::endl;
            throw; // Re-lança a exceção para tratamento adicional
        }
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

        diskManager.readBlock(superblock.root_dir_index, reinterpret_cast<char*>(rootDir.data()));
    
        for (const auto &entry : rootDir) {
            if (entry.filename[0] != '\0') {
                cout << "Filename: " << entry.filename << ", Type: " << entry.file_type << ", Index Block: " << entry.index_block << endl;
            }
        }
    
        diskManager.close();
    }

    //Listar os blocos livres
    //Ao listar os blocos livres estao aparecendo mais do que deviam, pois o superbloco deiz uma coisa e o listFreeBlocks diz outra
    void listFreeBlocks() {
        diskManager.open();
    
        diskManager.readBlock(superblock.bitmap_start, reinterpret_cast<char*>(bitmap.data()));

        //printar todo o bitmap
        for (uint32_t i = 0; i < superblock.total_blocks; i++) {
            cout << ((bitmap[i / 8] & (1 << (i % 8))) ? "1" : "0");
        }

        cout << " | ";

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

    void listFileDataBlocks(uint32_t index_block) {
        diskManager.open();
        
        IndexBlock ib;
        diskManager.readBlock(index_block, reinterpret_cast<char*>(&ib));
        
        cout << "Index Block: " << index_block << endl;
        cout << "Direct Pointers: ";
        for (const auto &ptr : ib.block_ptrs) {
            if (ptr != 0xFFFFFFFF) {
                cout << ptr << " ";
            }
        }
        cout << endl;
        cout << "Indirect Pointer: " << ib.indirect_ptr << endl;
        
        // Listar os blocos de dados diretos
        for (const auto &ptr : ib.block_ptrs) {
            if (ptr != 0xFFFFFFFF) {
                listDataBlock(ptr);
            }
        }
        
        // Listar os blocos de dados indiretos
        if (ib.indirect_ptr != 0xFFFFFFFF) {
            IndexBlock indirect_ib;
            diskManager.readBlock(ib.indirect_ptr, reinterpret_cast<char*>(&indirect_ib));
            
            for (const auto &ptr : indirect_ib.block_ptrs) {
                if (ptr != 0xFFFFFFFF) {
                    listDataBlock(ptr);
                }
            }
        }
        
        diskManager.close();
    }

    // Listar mapa de bits
    void listBitmap() {
        diskManager.open();
    
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