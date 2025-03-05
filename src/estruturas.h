#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <cmath>
#include <cstdint>

#define BLOCK_SIZE 512 //Tamanho do bloco em bytes

using namespace std;

/*
    Aqui ficarão defines relativos ao super bloco (bytes)
*/
#define TOTAL_BLOCKS_SIZE 4
#define BITMAP_START 4
#define ROOT_DIR_INDEX 4
#define FREE_BLOCKS 4
#define SUPERBLOCK_NUMBER 4
#define VERSION 4


#define ENTRY_SIZE 64 //Tamanhos do arquivos (64 bytes)
/*
    Aqui ficarão os defines relacionados a entrada do diretório raiz (bytes)
*/

#define FILENAME_SIZE 55 //Tamanho do nome de arquivo (54 caracteres + \0)
#define FILE_TYPE_SIZE 1 //Tamanho do campo do tipo de arquivo
#define INDEX_BLOCK_SIZE 4 //Tamanho do campo do Número do bloco de índice associado ao arquivo/diretório.
#define FILE_SIZE 4 //Tamanho do arquivo em bytes

/*
    Estruturas
*/

// Superbloco
struct Superblock{
    uint32_t total_blocks; //Numero total de blocos no sistema
    uint32_t bitmap_start; //Bloco inicial do bitmap (sempre 1).
    uint32_t bitmap_blocks; //Número de blocos ocupados pelo bitmap.
    uint32_t root_dir_index; //Bloco de índice do diretório raiz.
    uint32_t free_blocks; //Número de blocos livres restantes.
    // TODO - conversar com o Eduardo sobre o block_size
    uint32_t block_size; //log2(tamanho do bloco) - log2(512).
    uint32_t superblock_number; //Numero do bloco que contem o superbloco
    uint32_t version; //Versão do sistema de arquivos

    Superblock(): total_blocks(0), bitmap_start(1), bitmap_blocks(0), root_dir_index(0),
    free_blocks(0), block_size(BLOCK_SIZE), superblock_number(0), version(1) {}

};

// Entrada do diretório raiz
struct RootDirEntry{
    char filename[FILENAME_SIZE]; //Nome do arquivo/diretório.
    char  file_type; //Tipo de arquivo (0 para arquivo e 1 para diretório).
    uint32_t index_block; //Número do bloco de índice associado ao arquivo/diretório.
    uint32_t file_size; //Tamanho do arquivo em bytes.

    RootDirEntry(): file_type(0), index_block(0xFFFFFFFF), file_size(0) {
        memset(filename, 0, FILENAME_SIZE);
    }
};

struct IndexBlock{
    vector<uint32_t> block_ptrs;
    uint32_t indirect_ptr;

    IndexBlock() : indirect_ptr(0xFFFFFFFF) {
        block_ptrs.resize((BLOCK_SIZE / 4) - 1, 0xFFFFFFFF);
    }

};
