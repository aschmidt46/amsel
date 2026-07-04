#include "bus.h"
#include "test/logging.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>
extern "C"{
    #include <armdisasm.h>
}

using namespace gba;


// General Internal Memory
//   00000000-00003FFF   BIOS - System ROM         (16 KBytes)
//   00004000-01FFFFFF   Not used
//   02000000-0203FFFF   WRAM - On-board Work RAM  (256 KBytes) 2 Wait
//   02040000-02FFFFFF   Not used
//   03000000-03007FFF   WRAM - On-chip Work RAM   (32 KBytes)
//   03008000-03FFFFFF   Not used
//   04000000-040003FE   I/O Registers
//   04000400-04FFFFFF   Not used
// Internal Display Memory
//   05000000-050003FF   BG/OBJ Palette RAM        (1 Kbyte)
//   05000400-05FFFFFF   Not used
//   06000000-06017FFF   VRAM - Video RAM          (96 KBytes)
//   06018000-06FFFFFF   Not used
//   07000000-070003FF   OAM - OBJ Attributes      (1 Kbyte)
//   07000400-07FFFFFF   Not used
// External Memory (Game Pak)
//   08000000-09FFFFFF   Game Pak ROM/FlashROM (max 32MB) - Wait State 0
//   0A000000-0BFFFFFF   Game Pak ROM/FlashROM (max 32MB) - Wait State 1
//   0C000000-0DFFFFFF   Game Pak ROM/FlashROM (max 32MB) - Wait State 2
//   0E000000-0E00FFFF   Game Pak SRAM    (max 64 KBytes) - 8bit Bus width
//   0E010000-0FFFFFFF   Not used
// Unused Memory Area
//   10000000-FFFFFFFF   Not used (upper 4bits of address bus unused)

Byte* gba::Bus::accessMemory(Word addr)
{
    (void)addr;
    if(addr >= 0x02000000 && addr < 0x20040000){
        // WRAM (board)
        return wramBoard.data() + (addr - 0x02000000);
    }
    if(addr >= 0x03000000 && addr < 0x03008000){
        // WRAM (chip)
        return wramChip.data() + (addr - 0x03000000);
    }
    return &null;
}

void gba::Bus::writeByte(Word addr, Byte val)
{
    (void)addr;
    (void)val;

    if(addr >= 0x02000000 && addr < 0x03000000){
        // WRAM (board) + Mirror
        auto mod = (addr - 0x02000000) % 0x40000;
        wramBoard[mod] = val;
        return;
    }
    if(addr >= 0x03000000 && addr < 0x04000000){
        // WRAM (chip) + Mirror
        auto mod = (addr - 0x03000000) % 0x8000;
        wramChip[mod] = val;
        return;
    }

    if(addr >= 0x04000000 && addr < 0x04000060){
        ppu.writePPURegister(addr, val);
        return;
    }

    //Interrupt Handling
    if(addr == 0x04000200){
        IE = (IE & 0xFF00) | val;
        return;
    }
    if(addr == 0x04000201){//!!
        IE = (IE & 0x00FF) | (val << 8);
        return;
    }
    if(addr == 0x04000202){
        IF = IF & ~HalfWord(val);
        return;
    }
    if(addr == 0x04000203){
        IF = IF & ~(HalfWord(val) << 8);
        return;
    }
    if(addr == 0x04000208){
        IME = val;
        return;
    }

    if(addr >= 0x05000000 && addr < 0x08000000){
        ppu.writePPUMemory(addr, val);
        return;
    }

    // Cart Ram
    if(addr >= 0x0E000000 && addr < 0x0E010000){
        cartRam[addr - 0x0E000000] = val;
        return;
    }
    if(addr >= 0x0F000000 && addr < 0x0F010000){
        cartRam[addr - 0x0F000000] = val;
        return;
    }

    // std::cout << "Unbekannter Write: " << getHex(addr, 8) << std::endl;
}

Byte gba::Bus::readByte(Word addr)
{
    (void)addr;
    
    if(addr < 0x4000){
        return bios[addr];
    }
    if(addr >= 0x02000000 && addr < 0x03000000){
        // WRAM (board) + Mirror
        auto mod = (addr - 0x02000000) % 0x40000;
        return wramBoard[mod];
    }
    if(addr >= 0x03000000 && addr < 0x04000000){
        // WRAM (chip) + Mirror
        auto mod = (addr - 0x03000000) % 0x8000;
        return wramChip[mod];
    }


    if(addr >= 0x04000000 && addr < 0x04000060){
        return ppu.readPPURegister(addr);
    }

    //Interrupt Handling
    if(addr == 0x04000200){
        return IE;
    }
    if(addr == 0x04000201){
        return IE >> 8;
    }
    if(addr == 0x04000202){
        return IF;
    }
    if(addr == 0x04000203){
        return IF >> 8;
    }
    if(addr == 0x04000208){
        return IME;
    }

    // Ppu
    if(addr >= 0x05000000 && addr < 0x08000000){
        return ppu.readPPUMemory(addr);
    }

    // Rom Waitstate 0
    if(addr >= 0x08000000 && addr - 0x08000000 < gamePak.size()){
        return gamePak[addr - 0x08000000];
    }

    // Rom Waitstate 1
    if(addr >= 0x0A000000 && addr - 0x0A000000 < gamePak.size()){
        return gamePak[addr - 0x0A000000];
    }

    // Rom Waitstate 2
    if(addr >= 0x0C000000 && addr - 0x0C000000 < gamePak.size()){
        return gamePak[addr - 0x0C000000];
    }

    // Cart Ram
    if(addr >= 0x0E000000 && addr < 0x0E010000){
        return cartRam[addr - 0x0E000000];
    }
    if(addr >= 0x0F000000 && addr < 0x0F010000){
        return cartRam[addr - 0x0F000000];
    }

    // std::cout << "Unbekannter Read: " << getHex(addr, 8) << std::endl;

    return 0;
}

void gba::Bus::writeHalfWord(Word addr, HalfWord val)
{
    writeByte(addr, val & 0xFF);
    writeByte(addr + 1, (val & (0xFF << 8)) >> 8);
}

HalfWord gba::Bus::readHalfWord(Word addr)
{
    HalfWord A1 = readByte(addr);
    HalfWord A2 = readByte(addr + 1);
    return A1 | (A2 << 8);
}

void gba::Bus::writeWord(Word addr, Word val)
{
    writeByte(addr, val & 0xFF);
    writeByte(addr + 1, (val & (0xFF << 8)) >> 8);
    writeByte(addr + 2, (val & (0xFF << 16)) >> 16);
    writeByte(addr + 3, (val & (0xFF << 24)) >> 24);
}

Word gba::Bus::readWord(Word addr)
{
    Word A1 = readByte(addr);
    Word A2 = readByte(addr + 1);
    Word A3 = readByte(addr + 2);
    Word A4 = readByte(addr + 3);
    return A1 | (A2 << 8) | (A3 << 16) | (A4 << 24);
}

void gba::Bus::setIF(int bit, bool value) {
    IF = (IF & ~(1u << bit)) | (value << bit);
}

gba::Bus::Bus(){

    std::ifstream stream("../gbaroms/gba_bios.bin", std::ios::in | std::ios::binary);
    std::vector<uint8_t> contents((std::istreambuf_iterator<char>(stream)),
                                  std::istreambuf_iterator<char>());

  this->bios = contents;
  stream.close();
  this->wramBoard = std::vector<Byte>(0x40000, 0);
  this->wramChip = std::vector<Byte>(0x8000, 0);
  this->cartRam = std::vector<Byte>(0x10000, 0);
}

void gba::Bus::init() {
    for(int i = 0; i < 4; i++){
        timers.push_back(Timer(i, shared_from_this()));
    }
    ppu = PPU(shared_from_this());
    new (&cpu) CPU(shared_from_this());
}

gba::Bus::Bus(const char *path) : Bus() {
    std::filesystem::path p(path);

    std::ifstream stream(path, std::ios::in | std::ios::binary);
    std::vector<uint8_t> contents((std::istreambuf_iterator<char>(stream)),
                                  std::istreambuf_iterator<char>());

    this->gamePak = contents;
    stream.close();
}

gba::Bus::Bus(const std::vector<Byte> &bytes)  : Bus(){
    this->gamePak = bytes;
};


void gba::Bus::clock() {

    if(watchBreakpoints){
        if(std::find(breakpoints.begin(), breakpoints.end(), cpu.getRegisterState().R[15] - (cpu.state() == ARM ? 8 : 4)) != breakpoints.end()){
            halted = true;
        }
        if(std::find(breakpointsOP.begin(), breakpointsOP.end(), cpu.getCurrentOpcode()) != breakpointsOP.end()){
            halted = true;
        }
    }


    if(!halted || steps > 0){
        // Timers
        for(int i = 0; i < 4; i++){
            if(timers[i].usesPreviousTimer()){ // Bei t0 immer falsch
                if(timers[i-1].justOverflowed()){
                    timers[i].clockWithPrevious();
                }
            }
            else{
                timers[i].clock();
            }
        }
    
        cpu.clock();
        ppu.clock();

        if(steps > 0 && cpu.advancedThisClock()){
            steps--;
        }
    }

}

float *gba::Bus::accessFramebuffer()
{
    return ppu.accessFramebuffer();
}

bool gba::Bus::hasFrame() {
    return ppu.hasFrame();
}

void gba::Bus::setHalt(bool to) {
    halted = to;
}

bool gba::Bus::isHalted() {
    return halted;
}

void gba::Bus::addStep() {
    steps++;
}

std::pair<std::string, std::vector<int>> gba::Bus::getNextInstructions() {
    return cpu.getNextNInstructions(10);
}

std::pair<std::string, std::vector<int>> gba::Bus::getPrevInstructions() {
    return cpu.getPrev10Instructions();
}

std::vector<std::string> gba::Bus::removeBreakpointOP(std::string bp) {
    std::erase_if(breakpointsOP, [&](std::string i) { return i == bp; });


    if(watchBreakpoints && breakpointsOP.size() == 0 && breakpoints.size() == 0){
        watchBreakpoints = false;
    }
    return breakpointsOP;
}

std::vector<std::string> gba::Bus::addBreakpointOP(std::string bp) {
    if(!watchBreakpoints){
        watchBreakpoints = true;
    }

    bool exists = false;

    for(const auto &e : breakpointsOP){
        if(e==bp)
            exists = true;
    }

    if(!exists){
        breakpointsOP.push_back(bp);
    }

    return breakpointsOP;
}

CpuRegisterState gba::Bus::getRegs() {
    return cpu.getRegisterState();
}

std::vector<std::string> gba::Bus::getStack() {
    constexpr Word stackSize = 40; // Genug?

    Word sp = 0;
    auto state = cpu.getRegisterState();
    sp = state.R[13];
    if(cpu.mode() == Supervisor) sp = state.R_svc[0];
    else if(cpu.mode() == IRQ) sp = state.R_irq[0];

    // Stack wächst nach oben(!?)
    std::vector<std::string> stack;

    for(Word i = 0; i < stackSize; i++){
        Word val = readWord(sp + (4 * i));
        stack.push_back(getHex(val, 8));
    }

    return stack;
}

std::string gba::Bus::getMode() {
    return cpu.printMode();
}

std::vector<uint64_t> gba::Bus::addBreakpoint(uint64_t bp) {
  if(!watchBreakpoints){
        watchBreakpoints = true;
    }

    bool exists = false;

    for(const auto &e : breakpoints){
        if(e==bp)
            exists = true;
    }

    if(!exists){
        breakpoints.push_back(bp);
    }

    return breakpoints;
}

std::vector<uint64_t> gba::Bus::removeBreakpoint(uint64_t bp) {
  std::erase_if(breakpoints, [&](uint64_t i) { return i == bp; });


    if(watchBreakpoints && breakpointsOP.size() == 0 && breakpoints.size() == 0){
        watchBreakpoints = false;
    }
    return breakpoints;
}

void gba::Bus::setHalt() {
    halted = true;
}

std::string gba::Bus::getDisassembly(uint64_t code)
{
    ARMSTATE s;
    auto state = cpu.getRegisterState();
    disasm_init(&s, 0);
    int mode = ((state.CPSR >> 5) & 1u) ? 0 : 1;
    s.arm_mode = mode;
    if(mode == 1){
        disasm_arm(&s, code);
    }
    else{
        disasm_thumb(&s, uint16_t(code), uint16_t(code >> 16));
    }
    std::string text = s.text;
    disasm_cleanup(&s);
    return text;
}

std::tuple<std::string, std::string, std::string> gba::Bus::getLastTransaction()
{
    std::string action = "";
    if(cpu.lastTransactionWasRead){
        action = "R";
    }
    else action = "W";
    std::string addr = getHex(cpu.lastTransactionAddress, 8);
    std::string data = getHex(cpu.lastTransactionData, 8);
    return std::tuple<std::string, std::string, std::string>{action, addr, data};
}
