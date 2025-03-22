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
    string numBlocks = argv[2];

    try {
        // Inicializa o disco com 1000 blocos
        FileSystem fs(diskPath, stoi(numBlocks));

        // Lista o superbloco
        cout << "Superblock:" << endl;
        fs.listSuperblock();
        cout << endl;

        // Lista os blocos livres
        cout << "Free Blocks:" << endl;
        fs.listFreeBlocks();
        cout << endl;

        // Cria um arquivo
        string filename = "SUBDIR";
        fs.createFile(filename, '2'); // '2' para arquivo comum
        filename = "testfile.txt";
        fs.createFile(filename, '1'); // '1' para arquivo comum
        filename = "thomasturbano.txt";
        fs.createFile(filename, '1'); // '1' para arquivo comum

        //Criar um arquivo dentro de um diretório
        string parentDir = "SUBDIR";
        filename = "arquivoDentroDeDiretorio.txt";
        fs.createFile(filename, '1', parentDir); // '1' para arquivo comum

        cout<<endl;

        // Lista os arquivos no diretório raiz
        cout << "Root Directory:" << endl;
        fs.listFilesRecursively();
        cout << endl;

        cout << "Free Blocks:" << endl;
        fs.listFreeBlocks();
        cout << endl;

        //TODO: creio que ao escrever algum dado no arquivo, estamos enxendo ele de lixo
        // Escreve dados no arquivo
        // const char *data = "Hello, world! This is a test file.";
        // uint32_t dataSize = strlen(data);
        // uint32_t indexBlock;
        // char fileType;
        // fs.readFile(filename, &fileType, &indexBlock); // Obtém o bloco de índice do arquivo
        // fs.writeFile(filename, data, dataSize); // Escreve os dados no arquivo
 
        // Lista o bloco de índice do arquivo
        // cout << "Index Block of " << filename << ":" << endl;
        // //fs.listIndexBlock(indexBlock);
        // cout << endl;

        // // Lê os dados do arquivo
        // char readData[BLOCK_SIZE];
        // cout << "Ler dados do arquivo " << filename << ":" << endl;
        // fs.readFile(indexBlock, 0, readData, dataSize);
        // cout << "Data read from " << filename << ": " << readData << endl;
        // cout << endl;

        // // Lista o mapa de bits
        // cout << "Free Blocks:" << endl;
        // fs.listFreeBlocks();
        // cout << endl;

        // // Lista o disco (apenas os primeiros 10 blocos para simplificar)
        // cout << "Disk (first 10 blocks):" << endl;
        // for (uint32_t i = 2; i < stoi(numBlocks); i++) {
        //     char blockData[BLOCK_SIZE];
        //     fs.listDataBlock(i);
        // }
        // cout << endl;

        // // Exclui o arquivo
        // fs.deleteFile(filename);

        // // Lista os arquivos no diretório raiz após exclusão
        // cout << "Root Directory after deletion:" << endl;
        // fs.listFiles();
        // cout << endl;

        // // Lista os blocos livres após exclusão
        // cout << "Free Blocks after deletion:" << endl;
        // fs.listFreeBlocks();
        // cout << endl;

    } catch (const exception &e) {
        cerr << "Error: " << e.what() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/*
Compilar o programa
g++ -o main main.cpp -std=c++17

Executar o programa passando o caminho do disco
./filesystem_test /caminho/para/disk.img
*/