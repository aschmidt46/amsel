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
#include <array>
#include <string>
#include <numeric>

using namespace nds;

std::array<std::string, 32> nds9IRQs = {{
    "LCD V-Blank",
    "LCD H-Blank",
    "LCD V-Count",
    "Timer 0",
    "Timer 1",
    "Timer 2",
    "Timer 3",
    "Bit 7 (unused)",
    "DMA 0",
    "DMA 1",
    "DMA 2",
    "DMA 3",
    "Keypad",
    "GBA Slot",
    "Bit 14 (unused)",
    "Bit 15 (unused)",
    "IPC Sync",
    "IPC Send FIFO",
    "IPC Receive FIFO",
    "Game Card Transfer",
    "Game Card IREQ_MC",
    "Geometry Command FIFO",
    "Bit 22 (unused)",
    "Bit 23 (unused)",
    "Bit 24 (unused)",
    "Bit 25 (unused)",
    "Bit 26 (unused)",
    "Bit 27 (unused)",
    "Bit 28 (unused)",
    "Bit 29 (unused)",
    "Bit 30 (unused)",
    "Bit 31 (unused)",
}};

std::array<std::string, 32> nds7IRQs = {{
    "LCD V-Blank",
    "LCD H-Blank",
    "LCD V-Count",
    "Timer 0",
    "Timer 1",
    "Timer 2",
    "Timer 3",
    "SIO / RCNT / RTC",
    "DMA 0",
    "DMA 1",
    "DMA 2",
    "DMA 3",
    "Keypad",
    "GBA Slot",
    "Bit 14 (unused)",
    "Bit 15 (unused)",
    "IPC Sync",
    "IPC Send FIFO",
    "IPC Receive FIFO",
    "Game Card Transfer",
    "Game Card IREQ_MC",
    "Bit 21 (unused)",
    "Screens unfolding",
    "SPI Bus",
    "Wifi",
    "Bit 25 (unused)",
    "Bit 26 (unused)",
    "Bit 27 (unused)",
    "Bit 28 (unused)",
    "Bit 29 (unused)",
    "Bit 30 (unused)",
    "Bit 31 (unused)",
}};



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
    if(watchARM7) return bus7->removeBreakpointOP(bp);
    else return bus9->removeBreakpointOP(bp);
}

std::vector<std::string> SharedBus::addBreakpointOP(std::string bp) {
    if(watchARM7) return bus7->addBreakpointOP(bp);
    else return bus9->addBreakpointOP(bp);
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
    if(watchARM7) return bus7->addBreakpoint(bp);
    else return bus9->addBreakpoint(bp);
}

std::vector<uint64_t> SharedBus::removeBreakpoint(uint64_t bp) {
    if(watchARM7) return bus7->removeBreakpoint(bp);
    else return bus9->removeBreakpoint(bp);
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
    if(ImGui::Button("System State")){
        showMemoryState = !showMemoryState;
    }
    if(ImGui::Button("Start Log")){
        bus9->cpu.startLogging = true;
        bus7->cpu.startLogging = true;
    }

    if(showPPURegisters){
        ImGui::Begin("PPU Registers", &showPPURegisters, ImGuiWindowFlags_NoCollapse);

        if(ImGui::BeginTabBar("selectEngine")){

            if(ImGui::BeginTabItem("engine A")){
                lcd->engineA.LCDCONTROL.render();
                ImGui::Text("framebufferOffset: ");
                ImGui::SameLine();
                ImGui::Text(std::to_string(lcd->engineA.framebufferOffset).c_str());
                ImGui::EndTabItem();
            }

            if(ImGui::BeginTabItem("engine B")){
                lcd->engineB.LCDCONTROL.render();
                ImGui::Text("framebufferOffset: ");
                ImGui::SameLine();
                ImGui::Text(std::to_string(lcd->engineB.framebufferOffset).c_str());
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }



        ImGui::End();
    }

    if(showMemoryState){
        ImGui::Begin("System State", &showMemoryState, ImGuiWindowFlags_NoCollapse);

        if(ImGui::BeginTabBar("selectSubSystem")){

            if(ImGui::BeginTabItem("Memory")){

                ImGui::SeparatorText("TCM");
                bus9->cpu.cp15->renderTCM();

                ImGui::EndTabItem();
            }

            if(ImGui::BeginTabItem("IPC")){

                ImGui::SeparatorText("IPC");

                ImGui::Text("IPCFIFOCNT 9: ");
                ImGui::SameLine();
                ImGui::Text(getHex0x((HalfWord(ipc->readIPCFIFOCNTHigher(true)) << 8) | ipc->readIPCFIFOCNTLower(true), 4).c_str());

                ImGui::Text("IPCFIFOCNT 7: ");
                ImGui::SameLine();
                ImGui::Text(getHex0x((HalfWord(ipc->readIPCFIFOCNTHigher(false)) << 8) | ipc->readIPCFIFOCNTLower(false), 4).c_str());

                ImGui::EndTabItem();
            }

            if(ImGui::BeginTabItem("Interrupts")){

                ImGui::SeparatorText("IRQ");
                std::vector<std::string> ie7;
                std::vector<std::string> if7;
                std::vector<std::string> ie9;
                std::vector<std::string> if9;
                for(int i = 0; i < 32; i++){
                    if(((bus7->getIE() >> i) & 1u) > 0){
                        ie7.push_back(nds7IRQs[i]);
                    }
                }
                for(int i = 0; i < 32; i++){
                    if(((bus7->getIF() >> i) & 1u) > 0){
                        if7.push_back(nds7IRQs[i]);
                    }
                }
                for(int i = 0; i < 32; i++){
                    if(((bus9->getIE() >> i) & 1u) > 0){
                        ie9.push_back(nds9IRQs[i]);
                    }
                }
                for(int i = 0; i < 32; i++){
                    if(((bus9->getIF() >> i) & 1u) > 0){
                        if9.push_back(nds9IRQs[i]);
                    }
                }

                auto joinDelim = [](std::string ss, std::string s){ return ss.empty() ? s : ss + ", " + s; };

                std::string ie7s = std::accumulate(std::begin(ie7), std::end(ie7), std::string(), joinDelim);
                std::string if7s = std::accumulate(std::begin(if7), std::end(if7), std::string(), joinDelim);
                std::string ie9s = std::accumulate(std::begin(ie9), std::end(ie9), std::string(), joinDelim);
                std::string if9s = std::accumulate(std::begin(if9), std::end(if9), std::string(), joinDelim);

                ImGui::SeparatorText("NDS7");
                ImGui::Text("IME: ");
                ImGui::SameLine();
                ImGui::Text(std::to_string((int)bus7->hasIME()).c_str());
                
                ImGui::Text("IE: ");
                ImGui::SameLine();
                ImGui::Text(ie7s.c_str());
                
                ImGui::Text("IF: ");
                ImGui::SameLine();
                ImGui::Text(if7s.c_str());

                ImGui::SeparatorText("NDS9");
                ImGui::Text("IME: ");
                ImGui::SameLine();
                ImGui::Text(std::to_string((int)bus9->hasIME()).c_str());
                
                ImGui::Text("IE: ");
                ImGui::SameLine();
                ImGui::Text(ie9s.c_str());
                
                ImGui::Text("IF: ");
                ImGui::SameLine();
                ImGui::Text(if9s.c_str());
                

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
