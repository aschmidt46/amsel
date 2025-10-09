#include "cputest.h"
#include <fstream>
#include <iostream>
#include <chrono>
#include "mapper.h"

using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;
using std::chrono::microseconds;
using std::chrono::seconds;

//You'll eventually need to understand the iNES file format.
//For now, you can load 0x4000 bytes starting at offset 0x0010,
//and map that as ROM into both $8000-$BFFF and $C000-$FFFF of the emulated 6502's memory map.
//You can make an iNES parser once you start trying to actually run Concentration Room or Donkey Kong.

CPUTest::CPUTest(){
    NESFile* cart = new NESFile("nestest.nes");
    Ppu* ppu = new Ppu();
    cpu = new Cpu();
    Mapper* m = new Mapper(cart, cpu, ppu);
    auto finalize = [&](){
        delete cart;
        delete ppu;
        delete cpu;
        delete m;
    };
    cpu->init(0xC000, m);
    auto t0 = high_resolution_clock::now();
    while(true){
        auto t1 = high_resolution_clock::now();
        cpu->clockCPU();
        if(duration_cast<milliseconds>(t1-t0).count()>50)
            break;
    }

    std::string myline;
    std::string nesline;

    std::ifstream neslog("nestest.log", std::ios::in);
    std::ifstream mylog("cout.txt", std::ios::in);

    int line = 1;
    if(neslog.is_open() && mylog.is_open()){
        while(std::getline(neslog, nesline) && std::getline(mylog, myline)){
            LogLine nesLL = LogLine::parseNesTestLogLine(nesline);
            LogLine myLL = LogLine::parseMyLogLine(myline);

            if(nesLL!=myLL){
                std::cout << "Unstimmigkeit in Zeile " << line << ":" << std::endl;
                if(nesLL.PC != myLL.PC)
                    std::cout << "PC ungleich!" << std::endl;
                if(nesLL.opcode != myLL.opcode)
                    std::cout << "Opcode ungleich!" << std::endl;
                if(nesLL.A != myLL.A)
                    std::cout << "A ungleich!" << std::endl;
                if(nesLL.X != myLL.X)
                    std::cout << "X ungleich!" << std::endl;
                if(nesLL.Y != myLL.Y)
                    std::cout << "Y ungleich!" << std::endl;
                if(nesLL.P != myLL.P)
                    std::cout << "P ungleich!" << std::endl;
                if(nesLL.SP != myLL.SP)
                    std::cout << "SP ungleich!" << std::endl;
                if(nesLL.CYC != myLL.CYC)
                    std::cout << "CYC ungleich!" << std::endl;
                finalize();
                throw;
            }
            if(line>8991){
                std::cout << "NESTEST BESTANDEN!!! GRATULIERE!" << std::endl;    
            }
            line++;
        }
        if(line>8991){
            std::cout << "NESTEST BESTANDEN!!! GRATULIERE!" << std::endl;    
        }
        else{
            std::cout << "Programm zu kurz gelaufen?" << std::endl;    
        }
    }
    else {
        std::cout << "Fehler beim öffnen!" << std::endl;
    }
    finalize();

};