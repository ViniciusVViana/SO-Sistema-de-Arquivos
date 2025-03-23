// Alunos: Guilherme Deitos, Vinicius Viana e Vinicius Eduardo
#include "FileSystem.h" 

using namespace std;

void displayMenu() {
    cout << "===== Sistema de Arquivos =====" << endl;
    cout << "1. Criar Arquivo" << endl;
    cout << "2. Ler Arquivo" << endl;
    cout << "3. Escrever em Arquivo - Não funciona" << endl; 
    cout << "4. Deletar Arquivo" << endl;
    cout << "5. Listar Arquivos" << endl;
    cout << "6. Listar Blocos Livres" << endl;
    cout << "7. Listar Superbloco" << endl;
    cout << "8. Listar Bitmap" << endl;
    cout << "9. Listar Disco em Hexadecimal" << endl;
    cout << "10. Sair" << endl;
    cout << "==============================" << endl;
    cout << "Escolha uma opção: ";
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        cerr << "Uso: " << argv[0] << " <caminho_do_disco> <numero_de_blocos>" << endl;
        return EXIT_FAILURE;
    }

    string diskPath = argv[1]; // Obtém o caminho do disco do primeiro argumento
    string numBlocks = argv[2];


    FileSystem fs(diskPath, stoi(numBlocks));

    string choice;
    string filename;
    char filetype;
    uint32_t index_block;
    char data[BLOCK_SIZE];
    uint32_t size;

    do {
        displayMenu();
        cin >> choice;
        cin.ignore(); // Limpa o buffer do teclado

        switch (stoi(choice)) {
            case 1: {
                cout << "Digite o nome do arquivo: ";
                getline(cin, filename);
                cout << "Digite o tipo do arquivo (1 para arquivo, 2 para diretório): ";
                cin >> filetype;
                cin.ignore();
                fs.createFile(filename, filetype);
                break;
            }
            case 2: {
                cout << "Digite o nome do arquivo: ";
                getline(cin, filename);
                fs.readFile(filename, &filetype, &index_block);
                cout << "Tipo do arquivo: " << filetype << ", Bloco de índice: " << index_block << endl;
                break;
            }
            case 3: {
                cout << "Digite o nome do arquivo: ";
                getline(cin, filename);
                cout << "Digite os dados a serem escritos: ";
                cin.getline(data, BLOCK_SIZE);
                cout << "Digite o tamanho dos dados: ";
                cin >> size;
                cin.ignore();
                fs.writeFile(filename, data, size);
                break;
            }
            case 4: {
                cout << "Digite o nome do arquivo: ";
                getline(cin, filename);
                fs.deleteFile(filename);
                break;
            }
            case 5: {
                fs.listFilesRecursively();
                break;
            }
            case 6: {
                fs.listFreeBlocks();
                break;
            }
            case 7: {
                fs.listSuperblock();
                break;
            }
            case 8: {
                fs.listBitmap();
                break;
            }
            case 9: {
                fs.listDiskHex();
                break;
            }
            case 10: {
                cout << "Saindo..." << endl;
                break;
            }
            default: {
                cout << "Opção inválida! Tente novamente." << endl;
                break;
            }
        }
        cin.get();
    } while (stoi(choice) != 10);

    return 0;
}

/*
    Compilar: g++ -o nome_arq main.cpp -std=c++17
    Executar: ./nome_arq <caminho_do_disco> <numero_de_blocos>
*/