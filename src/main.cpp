#include <iostream>
#include "FileSystem.h"

using namespace std;

int main(int argc, char *argv[]) {
    // Verifica se o caminho do disco foi fornecido
    if (argc < 3) {
        cerr << "Uso: " << argv[0] << " <caminho_do_disco> <numero_de_blocos>" << endl;
        return EXIT_FAILURE;
    }

    string diskPath = argv[1]; // Obtém o caminho do disco do primeiro argumento
    string numBlocos = argv[2];

    try {
        FileSystem fs(diskPath);

        // Inicializa o disco com 1000 blocos
        fs.init(stoi(numBlocos));

        // Lista o superbloco
        cout << "Superblock:" << endl;
        fs.listSuperblock();
        cout << endl;

        // Lista os blocos livres
        cout << "Free Blocks:" << endl;
        fs.listFreeBlocks();
        cout << endl;

        // Cria um arquivo
        string filename = "testfile.txt";
        fs.createFile(filename, 'F'); // 'F' para arquivo comum

        // Lista os arquivos no diretório raiz
        cout << "Root Directory:" << endl;
        fs.listFiles();
        cout << endl;

        // Escreve dados no arquivo
        const char *data = "Hello, world! This is a test file.";
        uint32_t dataSize = strlen(data);
        uint32_t indexBlock;
        char fileType;
        fs.readFile(filename, &fileType, &indexBlock); // Obtém o bloco de índice do arquivo
        fs.writeFile(indexBlock, 0, data, dataSize);

        // Lista o bloco de índice do arquivo
        cout << "Index Block of " << filename << ":" << endl;
        fs.listIndexBlock(indexBlock);
        cout << endl;

        // Lê os dados do arquivo
        char readData[BLOCK_SIZE];
        fs.readFile(indexBlock, 0, readData, dataSize);
        cout << "Data read from " << filename << ": " << readData << endl;
        cout << endl;

        // Lista o mapa de bits
        cout << "Bitmap:" << endl;
        fs.listBitmap();
        cout << endl;

        // Lista o disco (apenas os primeiros 10 blocos para simplificar)
        cout << "Disk (first 10 blocks):" << endl;
        for (uint32_t i = 0; i < 10; i++) {
            char blockData[BLOCK_SIZE];
            fs.listDataBlock(i);
        }
        cout << endl;

        // Exclui o arquivo
        fs.deleteFile(filename);

        // Lista os arquivos no diretório raiz após exclusão
        cout << "Root Directory after deletion:" << endl;
        fs.listFiles();
        cout << endl;

        // Lista os blocos livres após exclusão
        cout << "Free Blocks after deletion:" << endl;
        fs.listFreeBlocks();
        cout << endl;

    } catch (const exception &e) {
        cerr << "Error: " << e.what() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/*
# Compilar o programa
g++ -o filesystem_test main.cpp FileSystem.cpp DiskManager.cpp -std=c++17

# Executar o programa passando o caminho do disco
./filesystem_test /caminho/para/disk.img
*/