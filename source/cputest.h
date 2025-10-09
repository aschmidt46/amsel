#pragma once

#include "6502.h"
#include "nes_file.h"
#include <string>

struct LogLine{
    int PC;
    int opcode;
    int A;
    int X;
    int Y;
    int P;
    int SP;
    int CYC;

    bool operator==(LogLine a){
        return (    this->PC == a.PC
                &&  this->opcode == a.opcode
                &&  this->A == a.A
                &&  this->X == a.X
                &&  this->Y == a.Y
                &&  this->P == a.P
                &&  this->SP == a.SP
                &&  this->CYC == a.CYC
        );
    }

    static LogLine parseMyLogLine(std::string line){
        int pc, oc, a, x, y, p, sp, cyc;
        pc = std::stoi(line.substr(0, 4), 0, 16);
        oc = std::stoi(line.substr(14, 2), 0, 16);
        a = std::stoi(line.substr(26, 2), 0, 16);
        x = std::stoi(line.substr(31, 2), 0, 16);
        y = std::stoi(line.substr(36, 2), 0, 16);
        p = std::stoi(line.substr(41, 2), 0, 16);
        sp = std::stoi(line.substr(47, 2), 0, 16);
        cyc = std::stoi(line.substr(54, line.size() - 54), 0, 16);
        return {pc,oc,a,x,y,p,sp,cyc};
    }

    static LogLine parseNesTestLogLine(std::string line){
        int pc, oc, a, x, y, p, sp, cyc;
        pc = std::stoi(line.substr(0, 4), 0, 16);
        oc = std::stoi(line.substr(6, 2), 0, 16);
        a = std::stoi(line.substr(50, 2), 0, 16);
        x = std::stoi(line.substr(55, 2), 0, 16);
        y = std::stoi(line.substr(60, 2), 0, 16);
        p = std::stoi(line.substr(65, 2), 0, 16);
        sp = std::stoi(line.substr(71, 2), 0, 16);
        cyc = std::stoi(line.substr(78, line.size() - 78), 0, 16);
        return {pc,oc,a,x,y,p,sp,cyc};
    }
};

class CPUTest{
    public:
    Cpu* cpu;
    CPUTest();
};