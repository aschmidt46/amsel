#include "bus.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

using namespace gba;


int main(){
    std::cout << "los gehts" << std::endl;
    std::filesystem::path p("../gbaroms/emerald.gba");

    std::ifstream stream("../gbaroms/emerald.gba", std::ios::in | std::ios::binary);
    std::vector<uint8_t> contents((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    CPU cpu;
    

    uint8_t* rawData = new uint8_t[contents.size()];
    for(int i = 0; i < contents.size(); i++){
        rawData[i] = contents[i];
    }
    stream.close();


    for(int i= 0; i < contents.size(); i+=4){
        Word u1 = rawData[i];
        Word u2 = rawData[i+1];
        Word u3 = rawData[i+2];
        Word u4 = rawData[i+3];
        Word instruction = u1 | (u2 << 8) | (u3 << 16) | (u4 << 24);
        auto decoded = cpu.decodeInstruction(instruction);
        std::cout << CPU::printInstructionType(decoded.type) << std::endl;
        std::string a = "";
        std::cin >> a;
    }


    return 0;
}