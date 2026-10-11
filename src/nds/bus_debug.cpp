#include "bus.h"
#include "framework/stringlib.h"
extern "C"{
    #include <armdisasm.h>
}
#include <vector>
//#ifdef BUILD_DESKTOP
#include <imgui.h>
#include <iostream>
//#endif

using namespace nds;



void SharedBus::setDebug(bool to)
{
    bus7->cpu.debug = to;
    bus9->cpu.debug = to;
}

void SharedBus::setHalt(bool to) {
    halted = to;
}

bool SharedBus::isHalted() {
    return halted;
}

void SharedBus::addStep() {
    steps++;
}

std::pair<std::string, std::vector<int>> SharedBus::getNextInstructions() {
    if(watchARM7)
        return bus7->cpu.getNextNInstructions(10);
    else
        return bus9->cpu.getNextNInstructions(10);
}

std::pair<std::string, std::vector<int>> SharedBus::getPrevInstructions() {
    if(watchARM7)
        return bus7->cpu.getPrev10Instructions();
    else
        return bus9->cpu.getPrev10Instructions();
}

std::vector<std::string> SharedBus::removeBreakpointOP(std::string bp) {
    std::erase_if(breakpointsOP, [&](std::string i) { return i == bp; });


    if(watchBreakpoints && breakpointsOP.size() == 0 && breakpoints.size() == 0){
        watchBreakpoints = false;
    }
    return breakpointsOP;
}

std::vector<std::string> SharedBus::addBreakpointOP(std::string bp) {
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

CpuRegisterState SharedBus::getRegs() {
    if(watchARM7)
        return std::bit_cast<CpuRegisterState>(bus7->cpu.getRegisterState());
    else
        return bus9->cpu.getRegisterState();
}

std::vector<std::string> SharedBus::getStack() {
    constexpr Word stackSize = 40; // Genug?

    Word sp = 0;
    auto state = watchARM7 ? std::bit_cast<CpuRegisterState>(bus7->cpu.getRegisterState()) : bus9->cpu.getRegisterState();
    sp = state.R[13];
    auto mode = watchARM7 ? std::bit_cast<nds::OperatingMode>(bus7->cpu.mode()) : bus9->cpu.mode();
    if(mode == Supervisor) sp = state.R_svc[0];
    else if(mode == IRQ) sp = state.R_irq[0];

    // Stack wächst nach oben(!?)
    std::vector<std::string> stack;

    for(Word i = 0; i < stackSize; i++){
        Word val = watchARM7 ? bus7->readWord(sp + (4 * i)) : bus9->readWord(sp + (4 * i));
        stack.push_back(getHex0x(val, 8));
    }

    return stack;
}

std::string SharedBus::getMode() {
    return watchARM7 ? bus7->cpu.printMode() : bus9->cpu.printMode();
}
std::string SharedBus::getState() {
    auto s = watchARM7 ? std::bit_cast<nds::CpuState>(bus7->cpu.state()) : bus9->cpu.state();
    if(s == ARM) return "ARM";
    else return "THUMB";
}

std::vector<uint64_t> SharedBus::addBreakpoint(uint64_t bp) {
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

std::vector<uint64_t> SharedBus::removeBreakpoint(uint64_t bp) {
  std::erase_if(breakpoints, [&](uint64_t i) { return i == bp; });


    if(watchBreakpoints && breakpointsOP.size() == 0 && breakpoints.size() == 0){
        watchBreakpoints = false;
    }
    return breakpoints;
}

std::string SharedBus::getDisassembly(uint64_t code)
{
    ARMSTATE s;
    auto state = watchARM7 ? std::bit_cast<CpuRegisterState>(bus7->cpu.getRegisterState()) : bus9->cpu.getRegisterState();
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
    if(watchARM7){
        auto info = bus7->cpu.decodeInstruction(code);
        text += " | " + bus7->cpu.printInstructionType(info.type);
    }
    else{
        auto info = bus9->cpu.decodeInstruction(code);
        text += " | " + bus9->cpu.printInstructionType(info.type);
    }
    return text;
}

std::tuple<std::string, std::string, std::string> SharedBus::getLastTransaction()
{
    std::string action = "";
    if(watchARM7 && bus7->cpu.lastTransactionWasRead){
        action = "R";
    }
    if(!watchARM7 && bus9->cpu.lastTransactionWasRead){
        action = "R";
    }
    else action = "W";
    std::string addr = watchARM7 ? getHex0x(bus7->cpu.lastTransactionAddress, 8) : getHex0x(bus9->cpu.lastTransactionAddress, 8);
    std::string data = watchARM7 ? getHex0x(bus7->cpu.lastTransactionData, 8) : getHex0x(bus9->cpu.lastTransactionData, 8);
    return std::tuple<std::string, std::string, std::string>{action, addr, data};
}

void nds::SharedBus::extendedDebugMenu()
{
    //#ifdef BUILD_DESKTOP

    if(ImGui::Button("Create Memdump NDS9")){
        std::ofstream out("memdump-nds9.txt");
        memdump9(out);
    }
    if(ImGui::Button("PPU Registers")){
        showPPURegisters = !showPPURegisters;
    }

    if(showPPURegisters){
        ImGui::Begin("PPU Registers", &showPPURegisters, ImGuiWindowFlags_NoCollapse);

        if(ImGui::BeginTabBar("selectEngine")){

            if(ImGui::BeginTabItem("engine A")){
                lcd->engineA.LCDCONTROL.render();
                ImGui::EndTabItem();
            }

            if(ImGui::BeginTabItem("engine B")){
                lcd->engineB.LCDCONTROL.render();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }



        ImGui::End();
    }


    //#endif
}

void nds::SharedBus::memdump9(std::ofstream &out)
{
    out << "----ITCM----\n";
    Word addr = 0;
    for(const auto &val : bus9->getITCM()){
        out << getHex0x(addr, 8) << getHex0x(val, 2) << "\n";
        addr++;
    }

    out << "----DTCM----\n";
    addr = 0;
    for(const auto &val : bus9->getDTCM()){
        out << getHex0x(addr, 8) << getHex0x(val, 2) << "\n";
        addr++;
    }

    out << "----Main Memory----\n";
    addr = 0x02000000;
    for(const auto &val : mainMemory){
        out << getHex0x(addr, 8) << getHex0x(val, 2) << "\n";
        addr++;
    }

    out << "----Shared WRAM----\n";
    addr = 0x03000000;
    for(const auto &val : sharedWRAM){
        out << getHex0x(addr, 8) << getHex0x(val, 2) << "\n";
        addr++;
    }

    std::cout << "memdump finished\n";
}
