#include <iostream>
#include <set>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <cmath>
#include <cstdint>
#include "estruturas.h"

// Criar a função min
template <typename T>
T getMin(T a, T b)
{
    return a < b ? a : b;
}

class FileSystem
{
private:
    fstream disk;
    Superblock superblock;
    vector<uint8_t> bitmap;
    vector<RootDirEntry> rootDir;
    string diskPath;

    // Calcular numero de blocos para o bitmap
    u_int32_t calcNumBlocksBitmap(u_int32_t numBlocks)
    {
        return (numBlocks + BLOCK_SIZE * 8 - 1) / (BLOCK_SIZE * 8);
    }

    // Gerenciador de disco
    class DiskManager
    {
    private:
        bool openFlag = false;
        string diskPath;

    public:
        fstream disk;

        DiskManager() = default; // Construtor padrão

        DiskManager(string &path)
        {
            diskPath = path;
        }

        void create(u_int32_t size)
        {
            disk.open(diskPath, ios::in | ios::out | ios::binary);
            if (!disk.is_open())
            {
                throw runtime_error("Erro ao criar o disco");
            }
            disk.seekp(size - 1);
            if (!disk)
            {
                throw runtime_error("Erro ao posicionar o ponteiro de escrita ao criar o disco");
            }
            disk.write("", 1);
            if (!disk)
            {
                throw runtime_error("Erro ao escrever no disco ao criar o disco");
            }
            disk.close();
        }

        /* void open(){
            disk.open(diskPath, ios::in | ios::out | ios::binary);
            if(!disk.is_open()){
                throw runtime_error("Erro ao abrir o disco!");
            }
            openFlag = true;
        }

        void close(){
            if(disk.is_open()){
                disk.close();
                openFlag = false;
                return;
            }
            cout<<"O gerenciador de disco ja estava fechado!"<<endl;
        } */

        void readBlock(u_int32_t blockIndex, char *data)
        {
            disk.open(diskPath, ios::in | ios::out | ios::binary);
            if (disk.is_open())
            {
                disk.seekg(blockIndex * BLOCK_SIZE);
                disk.read(data, BLOCK_SIZE);
                // cout << "Leitura finalizada!" << endl;
                disk.close();
                return;
            }
            throw runtime_error("Erro ao abrir e ler o disco!");
        }

        void writeBlock(u_int32_t blockIndex, char *data)
        {
            disk.open(diskPath, ios::in | ios::out | ios::binary);
            if (disk.is_open())
            {
                disk.seekp(blockIndex * BLOCK_SIZE);
                disk.write(data, BLOCK_SIZE);
                // cout << "Escrita finalizada!" << endl;
                disk.close();
                return;
            }
            throw runtime_error("Erro ao abrir e escrever no disco");
        }
    };

    DiskManager diskManager;

    void listFilesInDirectory(uint32_t dirIndexBlock, string path)
    {
        IndexBlock ib;
        cout << "Listando arquivos no diretório: " << path << endl;

        try
        {
            diskManager.readBlock(dirIndexBlock, reinterpret_cast<char *>(&ib));
        }
        catch (const std::exception &e)
        {
            cerr << "Erro ao ler o bloco de índice: " << e.what() << endl;
            return;
        }

        cout << "Listando arquivos no diretório: " << path << endl;


        for (const auto &ptr : ib.block_ptrs)
        {
            if (ptr != 0xFFFFFFFF)
            {
                RootDirEntry dirEntry;
                try
                {
                    diskManager.readBlock(ptr, reinterpret_cast<char *>(&dirEntry));
                }
                catch (const std::exception &e)
                {
                    cerr << "Erro ao ler o bloco de diretório: " << e.what() << endl;
                    continue;
                }
                if (dirEntry.filename[0] != '\0') // Verifica se a entrada é válida
                {
                    
                    std::string fullPath = path + "/" + std::string(dirEntry.filename);
                    cout << "Filename: " << fullPath << ", Type: " << dirEntry.file_type << ", Index Block: " << dirEntry.index_block << endl;
                    if (dirEntry.file_type == '2') // Se for um diretório, listar recursivamente
                    {
                        listFilesInDirectory(dirEntry.index_block, fullPath);
                    }
                }
            }
        }
    }

public:
    FileSystem(string &path, u_int32_t numBlocks)
    {
        if (numBlocks < 4)
        {
            throw runtime_error("Número de blocos insuficiente!");
        }
        diskManager = DiskManager(path);

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
        diskManager.writeBlock(0, (char *)&superblock);
        for (u_int32_t i = 0; i < superblock.bitmap_start + superblock.bitmap_blocks + 1; i++)
        {
            bitmap[i / 8] |= 1 << (i % 8);
        }
        diskManager.writeBlock(superblock.bitmap_start, (char *)bitmap.data());
        diskManager.writeBlock(superblock.root_dir_index, (char *)rootDir.data());
        diskManager.writeBlock(1, (char *)bitmap.data());
    }

    // Alocar um bloco
    u_int32_t allocBlock()
    {

        if (superblock.free_blocks == 0)
        {
            return 0xFFFFFFFF; // Retorna erro se não houver blocos livres
        }

        cout << "Alocando bloco..." << endl;
        diskManager.readBlock(superblock.bitmap_start, (char *)bitmap.data());

        for (u_int32_t i = 0; i < superblock.total_blocks; i++)
        { // Começa do bloco 1
            if (!(bitmap[i / 8] & (1 << (i % 8))))
            {                                  // Se o bloco estiver livre
                bitmap[i / 8] |= 1 << (i % 8); // Marcar o bloco como ocupado
                superblock.free_blocks--;

                // Atualiza o bitmap e o superbloco no disco
                diskManager.writeBlock(superblock.bitmap_start, (char *)bitmap.data());
                diskManager.writeBlock(0, (char *)&superblock);

                cout << "Bloco alocado: " << i << endl;

                return i; // Retorna o bloco alocado
            }
        }

        return 0xFFFFFFFF; // Retorna erro se não houver blocos livres
    }

    // Liberar um bloco
    void freeBlock(u_int32_t blockIndex)
    {
        if (blockIndex >= superblock.total_blocks)
        {
            throw runtime_error("Bloco Inválido!");
        }

        diskManager.readBlock(superblock.bitmap_start, (char *)bitmap.data());

        bitmap[blockIndex / 8] &= ~(1 << (blockIndex % 8));
        superblock.free_blocks++;
        diskManager.writeBlock(superblock.bitmap_start, (char *)bitmap.data());
        diskManager.writeBlock(0, (char *)&superblock);
    }

    // Criar um arquivo no diretório raiz
    void createFile(string &filename, char filetype)
    {

        if (filename.size() > FILENAME_SIZE)
        {
            throw runtime_error("Nome do arquivo muito grande!");
        }

        uint32_t index_block = allocBlock();
        if (index_block == 0xFFFFFFFF)
        {
            throw runtime_error("Não há blocos disponíveis!");
        }

        RootDirEntry newEntry;
        strncpy(newEntry.filename, filename.c_str(), FILENAME_SIZE);
        newEntry.filename[FILENAME_SIZE] = '\0';
        newEntry.file_type = filetype;
        newEntry.index_block = index_block;

        diskManager.readBlock(superblock.root_dir_index, (char *)rootDir.data());

        for (auto &entry : rootDir)
        {
            if (entry.filename[0] == '\0')
            {
                entry = newEntry;
                diskManager.writeBlock(superblock.root_dir_index, (char *)rootDir.data());
                cout << "Arquivo criado com sucesso!" << endl;
                return;
            }
        }

        throw runtime_error("Diretório raiz cheio!");
    }

    // Criar um arquivo ou diretório dentro de um diretório que não seja o raiz
    void createFile(string &filename, char filetype, string &parentDir)
    {

        if (filename.size() > FILENAME_SIZE)
        {
            throw runtime_error("Nome do arquivo muito grande!");
        }

        uint32_t index_block = allocBlock();
        if (index_block == 0xFFFFFFFF)
        {
            throw runtime_error("Não há blocos disponíveis!");
        }

        RootDirEntry newEntry;
        strncpy(newEntry.filename, filename.c_str(), FILENAME_SIZE);
        newEntry.filename[FILENAME_SIZE] = '\0';
        newEntry.file_type = filetype;
        newEntry.index_block = index_block;

        IndexBlock ib;
        diskManager.readBlock(superblock.root_dir_index, (char *)rootDir.data());

        uint32_t parentIndexBlock = 0;
        for (auto &entry : rootDir)
        {
            if (strcmp(entry.filename, parentDir.c_str()) == 0)
            {
                parentIndexBlock = entry.index_block;
                break;
            }
        }

        if (parentIndexBlock == 0)
        {
            throw runtime_error("Diretório pai não encontrado!");
        }

        for (auto &entry : ib.block_ptrs)
        {
            if (entry == 0xFFFFFFFF)
            {
                entry = index_block;
                diskManager.writeBlock(parentIndexBlock, (char *)&ib);
                cout << "Arquivo criado com sucesso!" << endl;
                return;
            }
        }

        throw runtime_error("Diretório cheio!");
    }

    // Ler um arquivo do diretório raiz
    void readFile(string &filename, char *filetype, u_int32_t *index_block)
    {

        diskManager.readBlock(superblock.root_dir_index, (char *)rootDir.data());

        for (auto &entry : rootDir)
        {
            if (strcmp(entry.filename, filename.c_str()) == 0)
            {
                *filetype = entry.file_type;
                *index_block = entry.index_block;
                return;
            }
        }

        // Verificar se o arquivo está em um diretório
        IndexBlock ib;
        for (auto &entry : rootDir)
        {
            if (entry.file_type == '2')
            {
                diskManager.readBlock(entry.index_block, (char *)&ib);
                for (auto &block_ptr : ib.block_ptrs)
                {
                    if (block_ptr != 0xFFFFFFFF)
                    {
                        RootDirEntry dirEntry;
                        diskManager.readBlock(block_ptr, (char *)&dirEntry);
                        if (strcmp(dirEntry.filename, filename.c_str()) == 0)
                        {
                            *filetype = dirEntry.file_type;
                            *index_block = dirEntry.index_block;
                            return;
                        }
                    }
                }
            }
        }

        throw runtime_error("Arquivo não encontrado!");
    }

    // Excluir um arquivo do diretório raiz
    void deleteFile(string &filename)
    {

        diskManager.readBlock(superblock.root_dir_index, (char *)rootDir.data());

        for (auto &entry : rootDir)
        {
            if (strcmp(entry.filename, filename.c_str()) == 0)
            {
                IndexBlock ib;
                diskManager.readBlock(entry.index_block, (char *)&ib);

                // Libera os blocos de dados
                for (auto ptr : ib.block_ptrs)
                {
                    if (ptr != 0xFFFFFFFF)
                    {
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
                diskManager.writeBlock(superblock.root_dir_index, (char *)rootDir.data());

                return;
            }
        }

        throw runtime_error("Arquivo não encontrado!");
    }
    // TODO: fazer essa merda funcionar
    // Escrever em um arquivo
    void writeFile(const string &filename, const char *data, uint32_t size)
    {
        try
        {
            // Abre o gerenciador de disco
            IndexBlock rootIndexBlock;

            // Pegar a partir do próximo bloco após o diretorio raiz
            diskManager.readBlock(superblock.root_dir_index + 1, reinterpret_cast<char *>(&rootIndexBlock));

            // Definir o tamanho do bloco de ponteiros
            uint32_t block_ptrs_size = BLOCK_SIZE / sizeof(uint32_t);

            // Verificar quantos ponteiros serão necessários para armazenar o arquivo
            uint32_t num_blocks = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;

            // Redefinir o tamanho do bloco de ponteiros
            if (size > block_ptrs_size * BLOCK_SIZE)
            {
                throw runtime_error("Tamanho do arquivo excede o limite de armazenamento");
            }

            // Recriar vetor de ponteiros
            rootIndexBlock.block_ptrs.resize(block_ptrs_size, 0xFFFFFFFF);

            // Definir ponteiro indireto
            rootIndexBlock.indirect_ptr = 0xFFFFFFFF;

            // Verificar quantos blocos de dados serão necessários
            uint32_t num_data_blocks = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;

            // Definir o bloco de dados atual
            uint32_t current_data_block = allocBlock();

            // cout << "Current Data Block: " << current_data_block << endl;

            // Exibir as informações do bloco de índice
            cout << "Index Block: " << superblock.root_dir_index << endl;
            cout << "Direct Pointers: " << rootIndexBlock.block_ptrs.size();
            cout << "Indirect Pointer: " << rootIndexBlock.indirect_ptr << endl;
        }
        catch (const exception &e)
        {
            cerr << e.what() << endl;
        }
    }

    // Ler de um arquivo
    void readFile(uint32_t index_block, uint32_t block_offset, char *data, uint32_t size)
    {

        IndexBlock ib;
        diskManager.readBlock(index_block, reinterpret_cast<char *>(&ib));

        uint32_t current_block_offset = block_offset;

        // Função auxiliar para ler dados de um bloco de índice
        auto readFromIndexBlock = [&](IndexBlock &ib, uint32_t &current_block_offset, char *&data, uint32_t &size)
        {
            for (uint32_t i = current_block_offset; i < ib.block_ptrs.size(); i++)
            {
                if (size == 0)
                {
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
        while (size > 0)
        {
            // Lê o bloco de índice indireto
            IndexBlock indirect_ib;
            diskManager.readBlock(ib.indirect_ptr, reinterpret_cast<char *>(&indirect_ib));

            // Lê dos ponteiros diretos do bloco de índice indireto
            readFromIndexBlock(indirect_ib, current_block_offset, data, size);
        }
    }

    // Listar os arquivos do diretório raiz
    void listFilesRecursively()
    {
        // Começar listando os arquivos no diretório raiz
        diskManager.readBlock(superblock.root_dir_index, reinterpret_cast<char *>(rootDir.data()));
        for (const auto &entry : rootDir)
        {
            if (entry.filename[0] != '\0')
            {
                std::string fullPath = "/" + std::string(entry.filename);
                cout << "Filename: " << fullPath << ", Type: " << entry.file_type << ", Index Block: " << entry.index_block << endl;
                if (entry.file_type == '2') // Se for um diretório, listar recursivamente
                {
                    cout << "Entrando no diretório: " << fullPath << endl;
                    listFilesInDirectory(entry.index_block, fullPath);
                }
            }
        }
    }
    // Listar os blocos livres
    // Ao listar os blocos livres estao aparecendo mais do que deviam, pois o superbloco deiz uma coisa e o listFreeBlocks diz outra
    void listFreeBlocks()
    {

        diskManager.readBlock(superblock.bitmap_start, reinterpret_cast<char *>(bitmap.data()));

        // printar todo o bitmap
        for (uint32_t i = 0; i < superblock.total_blocks; i++)
        {
            cout << ((bitmap[i / 8] & (1 << (i % 8))) ? "1" : "0");
        }

        cout << " | ";

        for (uint32_t i = 0; i < superblock.total_blocks; i++)
        {
            if (!(bitmap[i / 8] & (1 << (i % 8))) && i != 0)
            {
                cout << i << " ";
            }
        }

        cout << endl;
    }

    // Listar o superbloco
    void listSuperblock()
    {

        diskManager.readBlock(0, reinterpret_cast<char *>(&superblock));

        cout << "Total Blocks: " << superblock.total_blocks << endl;
        cout << "Bitmap Blocks: " << superblock.bitmap_blocks << endl;
        cout << "Root Directory Index: " << superblock.root_dir_index << endl;
        cout << "Free Blocks: " << superblock.free_blocks << endl;
    }

    // Listar o bloco de índice
    void listIndexBlock(uint32_t index_block)
    {

        IndexBlock ib;
        diskManager.readBlock(index_block, reinterpret_cast<char *>(&ib));

        cout << "Index Block: " << index_block << endl;
        cout << "Direct Pointers: ";
        for (const auto &ptr : ib.block_ptrs)
        {
            cout << ptr << " ";
        }
        cout << endl;
        cout << "Indirect Pointer: " << ib.indirect_ptr << endl;
    }

    // Listar um bloco de dados
    void listDataBlock(uint32_t data_block)
    {

        char data[BLOCK_SIZE];
        diskManager.readBlock(data_block, data);

        cout << "Data Block: " << data_block << endl;
        cout << "Data: " << data << endl;
    }

    void listFileDataBlocks(uint32_t index_block)
    {

        IndexBlock ib;
        diskManager.readBlock(index_block, reinterpret_cast<char *>(&ib));

        cout << "Index Block: " << index_block << endl;
        cout << "Direct Pointers: ";
        for (const auto &ptr : ib.block_ptrs)
        {
            if (ptr != 0xFFFFFFFF)
            {
                cout << ptr << " ";
            }
        }
        cout << endl;
        cout << "Indirect Pointer: " << ib.indirect_ptr << endl;

        // Listar os blocos de dados diretos
        for (const auto &ptr : ib.block_ptrs)
        {
            if (ptr != 0xFFFFFFFF)
            {
                listDataBlock(ptr);
            }
        }

        // Listar os blocos de dados indiretos
        if (ib.indirect_ptr != 0xFFFFFFFF)
        {
            IndexBlock indirect_ib;
            diskManager.readBlock(ib.indirect_ptr, reinterpret_cast<char *>(&indirect_ib));

            for (const auto &ptr : indirect_ib.block_ptrs)
            {
                if (ptr != 0xFFFFFFFF)
                {
                    listDataBlock(ptr);
                }
            }
        }
    }

    // Listar mapa de bits
    void listBitmap()
    {

        diskManager.readBlock(superblock.bitmap_start, reinterpret_cast<char *>(bitmap.data()));

        for (uint32_t i = 0; i < superblock.total_blocks; i++)
        {
            cout << ((bitmap[i / 8] & (1 << (i % 8))) ? "1" : "0");
        }

        cout << endl;
    }

    // Listar o disco
    void listDisk()
    {

        for (uint32_t i = 0; i < superblock.total_blocks; i++)
        {
            char data[BLOCK_SIZE];
            diskManager.readBlock(i, data);

            cout << "Block: " << i << endl;
            cout << "Data: " << data << endl;
        }
    }

    // Listar o disco em hexadecimal
    void listDiskHex()
    {

        for (uint32_t i = 0; i < superblock.total_blocks; i++)
        {
            char data[BLOCK_SIZE];
            diskManager.readBlock(i, data);

            cout << "Block: " << i << endl;
            cout << "Data: ";
            for (uint32_t j = 0; j < BLOCK_SIZE; j++)
            {
                cout << hex << (uint32_t)data[j] << " ";
            }
            cout << endl;
        }
    }
};