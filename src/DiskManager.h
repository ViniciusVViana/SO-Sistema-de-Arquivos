#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <cmath>
#include <cstdint>
#include "estruturas.h"

using namespace std;

class DiskManager{
    private:
        fstream disk;
        string path;
    public:
        DiskManager(string path): path(path){}

        void open(){
            cout << path << endl;
            disk.open(path, ios::in | ios::out | ios::binary);
            if(!disk.is_open()){
                throw std::runtime_error("Erro ao abrir o disco");
            }
        }

        void close(){
            if(disk.is_open()){
                disk.close();
            }
        }

        void create(int size){
            disk.open(path, ios::out | ios::binary);
            if(!disk.is_open()){
                throw std::runtime_error("Erro ao criar o disco");
            }
            disk.seekp(size - 1);
            disk.write("", 1);
            disk.close();
        }

        void writeBlock(uint32_t block_number, void* data){
            disk.seekp(block_number * BLOCK_SIZE, std::ios::beg);
            disk.write(reinterpret_cast<const char *>(data), BLOCK_SIZE);
        }

        void readBlock(uint32_t block_number, void* data){
            disk.seekg(block_number * BLOCK_SIZE, std::ios::beg);
            disk.read(reinterpret_cast<char *>(data), BLOCK_SIZE);
        }

        ~DiskManager(){
            close();
        }

};