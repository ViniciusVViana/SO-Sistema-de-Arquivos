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

    /**
     * @brief 
     * 
     * @param numBlocks Número de blocos do sistema de arquivos.
     * @return u_int32_t 
     */
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

        /**
         * @brief Construtor do gerenciador de disco.
         * 
         * @param path Caminho do disco.
         */
        DiskManager(string &path)
        {
            diskPath = path;
        }

        /**
         * @brief Cria o disco.
         * 
         * @param size Tamanho do disco.
         */
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

        /**
         * @brief Le um bloco do disco
         * 
         * @param blockIndex indice do bloco a ser lido
         * @param data 
         */
        void readBlock(u_int32_t blockIndex, char *data)
        {

            //Exibir os dados do bloco
            // Inicializar o buffer com zeros
            char buffer[BLOCK_SIZE];
            memset(buffer, 0x00, BLOCK_SIZE);
            disk.open(diskPath, ios::in | ios::out | ios::binary);
            if (disk.is_open())
            {
                disk.seekg(blockIndex * BLOCK_SIZE);
                //cout << "Lendo bloco: 0x" << blockIndex * BLOCK_SIZE << endl;
                disk.read(buffer, BLOCK_SIZE);
                //cout << endl;
                //Passar para a variavel data
                data = new char[BLOCK_SIZE];
                memcpy(data, buffer, BLOCK_SIZE);

                // cout << "Data lida: ";
                // for (int i = 0; i < BLOCK_SIZE; ++i)
                // {
                //     cout << hex << (int)data[i] << " ";
                // }

                if (disk.gcount() != BLOCK_SIZE)
                {
                    cerr << "Erro ao ler o bloco: tamanho lido diferente do esperado" << endl;
                }
                
                disk.close();
                return;
            }
            throw runtime_error("Erro ao abrir e ler o disco!");
        }

        /**
         * @brief Escreve um bloco no disco
         * 
         * @param blockIndex indice do bloco a ser escrito
         * @param data 
         */
        void writeBlock(u_int32_t blockIndex, char *data)
        {
            // Inicializar o buffer com zeros
            char buffer[BLOCK_SIZE];
            memset(buffer, 0x00, BLOCK_SIZE);

            // Copiar os dados para o buffer
            memcpy(buffer, data, BLOCK_SIZE);



            disk.open(diskPath, ios::in | ios::out | ios::binary);
            if (disk.is_open())
            {
                //cout << "Escrevendo bloco: 0x" << blockIndex * BLOCK_SIZE << endl;
                // cout << "Data a ser escrita: ";
                // for (int i = 0; i < BLOCK_SIZE; ++i)
                // {
                //     cout << hex << (int)buffer[i] << " ";
                // }
                // cout << endl;

                disk.seekp(blockIndex * BLOCK_SIZE);
                disk.write(buffer, BLOCK_SIZE);
                if (!disk)
                {
                    cerr << "Erro ao escrever no bloco" << endl;
                }
                disk.close();
                return;
            }
            throw runtime_error("Erro ao abrir e escrever no disco");
        }
    };

    DiskManager diskManager;

    /**
     * @brief Lista os arquivos em um diretório
     * 
     * @param dirIndexBlock
     * @param path 
     */
    void listFilesInDirectory(uint32_t dirIndexBlock, string path)
    {
        IndexBlock ib;
        cout << "Listando arquivos no diretório: " << path << endl;

        try
        {
            diskManager.readBlock(dirIndexBlock, reinterpret_cast<char *>(&ib));
        }
        catch (const exception &e)
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
                catch (const exception &e)
                {
                    cerr << "Erro ao ler o bloco de diretório: " << e.what() << endl;
                    continue;
                }
                if (dirEntry.filename[0] != '\0') // Verifica se a entrada é válida
                {

                    string fullPath = path + "/" + string(dirEntry.filename);
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
    /**
     * @brief Construtor do sistema de arquivos
     * 
     * @param path Caminho do disco
     * @param numBlocks Número de blocos do sistema de arquivos
     */
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
        for (u_int32_t i = 0; i < superblock.bitmap_start + superblock.bitmap_blocks + 1; i++)
        {
            bitmap[i / 8] |= 1 << (i % 8);
        }
        char buffer[BLOCK_SIZE];
        //Limpar buffer
        memset(buffer, 0x00, BLOCK_SIZE);
        //Escrever o superbloco no disco
        memcpy(buffer, &superblock, sizeof(Superblock));
        diskManager.writeBlock(0, buffer);
    
        diskManager.writeBlock(superblock.bitmap_start, (char *)bitmap.data());
        //Limpar buffer
        memset(buffer, 0x00, BLOCK_SIZE);
        //Escrever o diretório raiz no disco
        memcpy(buffer, rootDir.data(), rootDir.size());
        diskManager.writeBlock(superblock.root_dir_index, buffer);

        cout << "Sistema de arquivos criado com sucesso!" << endl;

    }

    /**
     * @brief Aloca um bloco livre no disco
     * 
     * @return Retornao ponteiro do bloco alocado
     */
    u_int32_t allocBlock()
    {

        if (superblock.free_blocks == 0)
        {
            return 0xFFFFFFFF; // Retorna erro se não houver blocos livres
        }
        char buffer[BLOCK_SIZE];

        diskManager.readBlock(superblock.bitmap_start, (char*)bitmap.data());

        for (u_int32_t i = 0; i < superblock.total_blocks; i++)
        { // Começa do bloco 1
            if (!(bitmap[i / 8] & (1 << (i % 8))))
            {                                  // Se o bloco estiver livre
                bitmap[i / 8] |= 1 << (i % 8); // Marcar o bloco como ocupado
                superblock.free_blocks--;

                diskManager.writeBlock(superblock.bitmap_start, (char *)bitmap.data());
                diskManager.writeBlock(0, (char *)&superblock);
                return i; // Retorna o bloco alocado
            }
        }

        return 0xFFFFFFFF; // Retorna erro se não houver blocos livres
    }

    /**
     * @brief Libera um bloco do disco
     * 
     * @param blockIndex indice do bloco a ser liberado
     */
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

        // Atualiza o superbloco no disco
        diskManager.writeBlock(0, (char *)&superblock);
    }

    
    /**
     * @brief Create a File object
     * 
     * @param filename O nome do arquivo ou pasta a ser criada.
     * @param filetype Indica o tipo do arquivo (2: pasta, 1: arquivo).
     * @param parentDir Caminho do diretório pai (deixar vazio para criar no diretório atual).
     */
    void createFile(string &filename, char filetype, const string &parentDir = "./")
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



        if (parentDir == "./")
        {
            diskManager.readBlock(superblock.root_dir_index, (char *)rootDir.data());
            for (auto &entry : rootDir)
            {
                if (entry.filename[0] == '\0')
                {
                    entry = newEntry;
                    IndexBlock ib;
                    for (auto &block_ptr : ib.block_ptrs)
                    {

                        if (block_ptr == 0xFFFFFFFF)
                        {
                            block_ptr = allocBlock();
                            cout << "Block Ptr: " << block_ptr << endl;
                            diskManager.writeBlock(entry.index_block, (char *)&ib);
                            diskManager.writeBlock(superblock.root_dir_index, (char *)rootDir.data());
                            cout << "Arquivo criado com sucesso!" << endl;
                            return;
                        }
                    }
                    
                }
            }
        }
        else
        {
            cout << "Parent Dir: " << parentDir << endl;
            IndexBlock ib;
            
            diskManager.readBlock(superblock.root_dir_index, (char *)rootDir.data());

            for (auto &entry : rootDir)
            {
                if (entry.file_type == '2' && strcmp(entry.filename, parentDir.c_str()) == 0)
                {
                    cout << "Index Block: " << entry.index_block << endl;
                    char buffer[BLOCK_SIZE];
                    diskManager.readBlock(entry.index_block, (char *)&buffer);

                    cout << "Buffer: " << endl;
                    for (int i = 0; i < BLOCK_SIZE; ++i)
                    {
                        cout << hex << (int)buffer[i] << " ";
                    }

                    cout << endl;



                    for (auto &block_ptr : ib.block_ptrs)
                    {
                        cout << "Block Ptr: " << block_ptr << endl;
                        if (block_ptr != 0xFFFFFFFF)
                        {
                            RootDirEntry dirEntry;
                            diskManager.readBlock(block_ptr, (char *)&dirEntry);
                            if (strcmp(dirEntry.filename, parentDir.c_str()) == 0)
                            {
                                diskManager.readBlock(dirEntry.index_block, (char *)&ib);
                                for (auto &block_ptr : ib.block_ptrs)
                                {
                                    if (block_ptr == 0xFFFFFFFF)
                                    {
                                        
                                        block_ptr = allocBlock();
                                        diskManager.writeBlock(dirEntry.index_block, (char *)&ib);
                                        diskManager.writeBlock(superblock.root_dir_index, (char *)rootDir.data());
                                        cout << "Arquivo criado com sucesso!" << endl;
                                        return;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            cout << ("Diretório cheio!") << endl;
        }
    }

    /**
     * @brief Le um arquivo do disco
     * 
     * @param filename Nome do arquivo a ser lido
     * @param filetype Tipo do arquivo a ser lido
     * @param index_block indice do bloco a ser lido
     */
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

        cout << ("Arquivo não encontrado!") << endl;
    }

    /**
     * @brief Retorna o índice do bloco de um arquivo
     *  Não funciona corretamente
     * @param filename Nome do arquivo
     * @return u_int32_t 
     */
    u_int32_t getFileBlockIndex(string &filename)
    {
        diskManager.readBlock(superblock.root_dir_index, (char *)rootDir.data());

        for (auto &entry : rootDir)
        {
            if (strcmp(entry.filename, filename.c_str()) == 0)
            {
                return entry.index_block;
            }
        }

        // Verificar se o arquivo está em um diretório
        IndexBlock ib;
        for (auto &entry : rootDir)
        {
            if (entry.file_type == '2')
            {
                cout << "Entry: " << entry.filename << endl;
                cout << "Index Block: " << hex << entry.index_block * BLOCK_SIZE<< endl;
                IndexBlock ib;
                diskManager.readBlock(entry.index_block, (char *) &ib);

                for (auto &block_ptr : ib.block_ptrs)
                {
                    if (block_ptr != 0xFFFFFFFF)
                    {
                        RootDirEntry dirEntry;
                        diskManager.readBlock(block_ptr, (char *)&dirEntry);
                        if (strcmp(dirEntry.filename, filename.c_str()) == 0)
                        {
                            return dirEntry.index_block;
                        }
                    }
                }

                

            }
        }

        cout << ("Arquivo não encontrado!") << endl;
        return 0xFFFFFFFF;
    }

    /**
     * @brief Retorna o índice do bloco de dados de um arquivo
     * Não funciona corretamente
     * @param index_block 
     * @param block_offset 
     * @return u_int32_t 
     */
    u_int32_t getFileDataBlockIndex(u_int32_t index_block, u_int32_t block_offset)
    {
        IndexBlock ib;
        diskManager.readBlock(index_block, (char *)&ib);

        if (block_offset < ib.block_ptrs.size())
        {
            return ib.block_ptrs[block_offset];
        }

        block_offset -= ib.block_ptrs.size();
        if (ib.indirect_ptr == 0xFFFFFFFF)
        {
            throw runtime_error("Bloco de dados não encontrado!");
        }

        diskManager.readBlock(ib.indirect_ptr, (char *)&ib);
        if (block_offset < ib.block_ptrs.size())
        {
            return ib.block_ptrs[block_offset];
        }

        throw runtime_error("Bloco de dados não encontrado!");
    }

    /**
     * @brief Deleta um arquivo do disco
     * 
     * @param filename Nome do arquivo
     */
    void deleteFile(string &filename)
    {
        cout << "Deletando arquivo: " << filename << endl;
        // Pegar o bloco de índice do arquivo
        u_int32_t indexBlock = getFileBlockIndex(filename);

        // Liberar o bloco de índice
        freeBlock(indexBlock);

        // Liberar do diretório raiz
        diskManager.readBlock(superblock.root_dir_index, (char *)rootDir.data());

        for (auto &entry : rootDir)
        {
            if (strcmp(entry.filename, filename.c_str()) == 0)
            {
                entry.filename[0] = '\0';
                char buffer[BLOCK_SIZE];
                //Limpar buffer
                memset(buffer, 0x00, BLOCK_SIZE);
                //Escrever o diretório raiz no disco
                memcpy(buffer, rootDir.data(), rootDir.size());
                diskManager.writeBlock(superblock.root_dir_index, buffer);
                cout << "Arquivo deletado com sucesso!" << endl;
                return;
            }
        }

        if (indexBlock == 0xFFFFFFFF)
        {
            cout << ("Arquivo não encontrado!") << endl;
        }

        // Problema na hora de deletar os dados do arquivo (ponteiros)

        // //Liberar os blocos de dados
        // IndexBlock ib;
        // diskManager.readBlock(indexBlock, (char *)&ib);

        // cout << "Index Block: " << ib << endl;

        // for (auto &block_ptr : ib.block_ptrs)
        // {
        //     if (block_ptr != 0xFFFFFFFF)
        //     {
        //         freeBlock(block_ptr);
        //     }
        // }

        // if (ib.indirect_ptr != 0xFFFFFFFF)
        // {
        //     diskManager.readBlock(ib.indirect_ptr, (char *)&ib);
        //     for (auto &block_ptr : ib.block_ptrs)
        //     {
        //         if (block_ptr != 0xFFFFFFFF)
        //         {
        //             freeBlock(block_ptr);
        //         }
        //     }
        //     freeBlock(ib.indirect_ptr);
        // }

        // //Remover a entrada do arquivo do diretório raiz
        // diskManager.readBlock(superblock.root_dir_index, (char *)rootDir.data());

        // for (auto &entry : rootDir)
        // {
        //     if (strcmp(entry.filename, filename.c_str()) == 0)
        //     {
        //         entry.filename[0] = '\0';
        //         diskManager.writeBlock(superblock.root_dir_index, (char *)rootDir.data());
        //         cout << "Arquivo deletado com sucesso!" << endl;
        //         return;
        //     }
        // }

        // // Verificar se o arquivo está em um diretório
        // IndexBlock ib2;
        // for (auto &entry : rootDir)
        // {
        //     if (entry.file_type == '2')
        //     {
        //         diskManager.readBlock(entry.index_block, (char *)&ib2);
        //         for (auto &block_ptr : ib2.block_ptrs)
        //         {
        //             if (block_ptr != 0xFFFFFFFF)
        //             {
        //                 RootDirEntry dirEntry;
        //                 diskManager.readBlock(block_ptr, (char *)&dirEntry);
        //                 if (strcmp(dirEntry.filename, filename.c_str()) == 0)
        //                 {
        //                     dirEntry.filename[0] = '\0';
        //                     diskManager.writeBlock(block_ptr, (char *)&dirEntry);
        //                     cout << "Arquivo deletado com sucesso!" << endl;
        //                     return;
        //                 }
        //             }
        //         }
        //     }
        // }
    }

    /**
     * @brief Busca por um arquivo no disco
     * 
     * @param filename Nome do arquivi a ser buscado
     */
    void searchFile(string &filename)
    {
        // Buscar o indice do arquivo
        u_int32_t indexBlock = getFileBlockIndex(filename);

        // Ler os dados do arquivo
        char fileType;
        u_int32_t indexBlock2;
        readFile(filename, &fileType, &indexBlock2);
        cout << "Arquivo: " << filename << " File Type: " << (fileType == '1' ? "Arquivo" : (fileType == '2' ? "Diretório" : "Tipo Desconhecido")) << ", Index Block: " << indexBlock2 << endl;
    }

    // TODO: fazer essa merda funcionar
    /**
     * @brief Escreve em um arquivo (NÃO FUNCIONA)
     * 
     * @param filename Nome do arquivo
     * @param data Dados a serem escritod no arquivo
     * @param size Tamanho do dados em bytes a serem escritos
     */
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

    /**
     * @brief Le um arquivo do disco
     * 
     * @param index_block indice do bloco do arquivo
     * @param block_offset 
     * @param data 
     * @param size 
     */
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

    /**
     * @brief Lista os arquivos do disco recursivamente
     *  Funciona parcialmente
     */
    void listFilesRecursively()
    {
        // Começar listando os arquivos no diretório raiz
        diskManager.readBlock(superblock.root_dir_index, reinterpret_cast<char *>(rootDir.data()));
        for (const auto &entry : rootDir)
        {
            if (entry.filename[0] != '\0')
            {
                string fullPath = "/" + string(entry.filename);
                cout << "Filename: " << fullPath << ", Type: " << (entry.file_type == '1' ? "Arquivo" : (entry.file_type == '2' ? "Diretório" : "Tipo Desconhecido")) << ", Index Block: " << entry.index_block << endl;
                // if (entry.file_type == '2') // Se for um diretório, listar recursivamente
                // {
                //     cout << "Entrando no diretório: " << fullPath << endl;
                //     listFilesInDirectory(entry.index_block, fullPath);
                // }
            }
        }
    }

    // Ao listar os blocos livres estao aparecendo mais do que deviam, pois o superbloco deiz uma coisa e o listFreeBlocks diz outra
    /**
     * @brief Lista os blocos livres do disco
     * 
     */
    void listFreeBlocks()
    {
        diskManager.readBlock(superblock.bitmap_start, reinterpret_cast<char *>(bitmap.data()));
       cout<<"Blocos livres: ";
        for (uint32_t i = 0; i < superblock.total_blocks; i++)
        {
            if (!(bitmap[i / 8] & (1 << (i % 8))) && i != 0)
            {
                cout << dec << i << " | ";
            }
        }
        cout << endl;
    }

    /**
     * @brief Lista os conteudos do superbloco do disco
     * 
     */
    void listSuperblock()
    {
        cout << &superblock << endl;
        diskManager.readBlock(0, reinterpret_cast<char *>(&superblock));

        cout << "Total Blocks: " << superblock.total_blocks << endl;
        cout << "Bitmap Blocks: " << superblock.bitmap_blocks << endl;
        cout << "Root Directory Index: " << superblock.root_dir_index << endl;
        cout << "Free Blocks: " << superblock.free_blocks << endl;
    }

    /**
     * @brief Lista o conteudo do bloco de indices
     * 
     * @param index_block 
     */
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

    /**
     * @brief Lista um bloco da dados
     * 
     * @param data_block 
     */
    void listDataBlock(uint32_t data_block)
    {

        char data[BLOCK_SIZE];
        diskManager.readBlock(data_block, data);

        cout << "Data Block: " << data_block << endl;
        cout << "Data: " << data << endl;
    }

    /**
     * @brief Lista o bitmap
     * 
     */
    void listBitmap()
    {

        diskManager.readBlock(superblock.bitmap_start, reinterpret_cast<char *>(bitmap.data()));

        for (uint32_t i = 0; i < superblock.total_blocks; i++)
        {
            cout << ((bitmap[i / 8] & (1 << (i % 8))) ? "1" : "0");
        }

        cout << endl;
    }

    /**
     * @brief Lista todo o disco
     * 
     */
    void listDisk()
    {

        for (uint32_t i = 0; i < superblock.total_blocks; i++)
        {
            char data[BLOCK_SIZE];
            diskManager.readBlock(i, data);

            cout << "Block: " << i << endl;

            cout << "Data: " << endl;

            for (uint32_t j = 0; j < BLOCK_SIZE; j++)
            {
                cout << dec << (uint32_t)data[j] << " ";
            }
        }
    }

    /**
     * @brief Lista o disco em hexadecimal
     * 
     */
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