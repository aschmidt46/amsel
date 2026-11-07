#include "gpu.h"
#include "../../bus.h"
#include <iostream>
#include "framework/stringlib.h"

using namespace nds;

void nds::GPU::updateIF()
{
    if(GXSTAT.state.commandFifoIRQ == 1){
        if(commandFifoSize < 128){
            bus->setIFSingleBus(21, true, true);
        }
    }
    else if(GXSTAT.state.commandFifoIRQ == 2){
        if(commandFifo.empty()){
            bus->setIFSingleBus(21, true, true);
        }
    }
}

void nds::GPU::fillMaps()
{

    addrToCmd[0x040] =  (GXCMD)0x10;
    addrToCmd[0x044] =  (GXCMD)0x11;
    addrToCmd[0x048] =  (GXCMD)0x12;
    addrToCmd[0x04C] =  (GXCMD)0x13;
    addrToCmd[0x050] =  (GXCMD)0x14;
    addrToCmd[0x054] =  (GXCMD)0x15;
    addrToCmd[0x058] =  (GXCMD)0x16;
    addrToCmd[0x05C] =  (GXCMD)0x17;
    addrToCmd[0x060] =  (GXCMD)0x18;
    addrToCmd[0x064] =  (GXCMD)0x19;
    addrToCmd[0x068] =  (GXCMD)0x1A;
    addrToCmd[0x06C] =  (GXCMD)0x1B;
    addrToCmd[0x070] =  (GXCMD)0x1C;
    addrToCmd[0x080] =  (GXCMD)0x20;
    addrToCmd[0x084] =  (GXCMD)0x21;
    addrToCmd[0x088] =  (GXCMD)0x22;
    addrToCmd[0x08C] =  (GXCMD)0x23;
    addrToCmd[0x090] =  (GXCMD)0x24;
    addrToCmd[0x094] =  (GXCMD)0x25;
    addrToCmd[0x098] =  (GXCMD)0x26;
    addrToCmd[0x09C] =  (GXCMD)0x27;
    addrToCmd[0x0A0] =  (GXCMD)0x28;
    addrToCmd[0x0A4] =  (GXCMD)0x29;
    addrToCmd[0x0A8] =  (GXCMD)0x2A;
    addrToCmd[0x0AC] =  (GXCMD)0x2B;
    addrToCmd[0x0C0] =  (GXCMD)0x30;
    addrToCmd[0x0C4] =  (GXCMD)0x31;
    addrToCmd[0x0C8] =  (GXCMD)0x32;
    addrToCmd[0x0CC] =  (GXCMD)0x33;
    addrToCmd[0x0D0] =  (GXCMD)0x34;
    addrToCmd[0x100] =  (GXCMD)0x40;
    addrToCmd[0x104] =  (GXCMD)0x41;
    addrToCmd[0x140] =  (GXCMD)0x50;
    addrToCmd[0x180] =  (GXCMD)0x60;
    addrToCmd[0x1C4] =  (GXCMD)0x71;
    addrToCmd[0x1C8] =  (GXCMD)0x72;


    cmdToNumParams[0x10] = 1;
    cmdToNumParams[0x11] = 0;
    cmdToNumParams[0x12] = 1;
    cmdToNumParams[0x13] = 1;
    cmdToNumParams[0x14] = 1;
    cmdToNumParams[0x15] = 0;
    cmdToNumParams[0x16] = 16;
    cmdToNumParams[0x17] = 12;
    cmdToNumParams[0x18] = 16;
    cmdToNumParams[0x19] = 12;
    cmdToNumParams[0x1A] = 9;
    cmdToNumParams[0x1B] = 3;
    cmdToNumParams[0x1C] = 3;
    cmdToNumParams[0x20] = 1;
    cmdToNumParams[0x21] = 1;
    cmdToNumParams[0x22] = 1;
    cmdToNumParams[0x23] = 2;
    cmdToNumParams[0x24] = 1;
    cmdToNumParams[0x25] = 1;
    cmdToNumParams[0x26] = 1;
    cmdToNumParams[0x27] = 1;
    cmdToNumParams[0x28] = 1;
    cmdToNumParams[0x29] = 1;
    cmdToNumParams[0x2A] = 1;
    cmdToNumParams[0x2B] = 1;
    cmdToNumParams[0x30] = 1;
    cmdToNumParams[0x31] = 1;
    cmdToNumParams[0x32] = 1;
    cmdToNumParams[0x33] = 1;
    cmdToNumParams[0x34] = 32;
    cmdToNumParams[0x40] = 1;
    cmdToNumParams[0x41] = 0;
    cmdToNumParams[0x50] = 1;
    cmdToNumParams[0x60] = 1;
    cmdToNumParams[0x70] = 3;
    cmdToNumParams[0x71] = 2;
    cmdToNumParams[0x72] = 1;


    cmdToFunction[0x10] = &GPU::MTX_MODE;
    cmdToFunction[0x11] = &GPU::MTX_PUSH;
    cmdToFunction[0x12] = &GPU::MTX_POP;
    cmdToFunction[0x13] = &GPU::MTX_STORE;
    cmdToFunction[0x14] = &GPU::MTX_RESTORE;
    cmdToFunction[0x15] = &GPU::MTX_IDENTITY;
    cmdToFunction[0x16] = &GPU::MTX_LOAD_4x4;
    cmdToFunction[0x17] = &GPU::MTX_LOAD_4x3;
    cmdToFunction[0x18] = &GPU::MTX_MULT_4x4;
    cmdToFunction[0x19] = &GPU::MTX_MULT_4x3;
    cmdToFunction[0x1A] = &GPU::MTX_MULT_3x3;
    cmdToFunction[0x1B] = &GPU::MTX_SCALE;
    cmdToFunction[0x1C] = &GPU::MTX_TRANS;
    cmdToFunction[0x20] = &GPU::COLOR;
    cmdToFunction[0x21] = &GPU::NORMAL;
    cmdToFunction[0x22] = &GPU::TEXCOORD;
    cmdToFunction[0x23] = &GPU::VTX_16;
    cmdToFunction[0x24] = &GPU::VTX_10;
    cmdToFunction[0x25] = &GPU::VTX_XY;
    cmdToFunction[0x26] = &GPU::VTX_XZ;
    cmdToFunction[0x27] = &GPU::VTX_YZ;
    cmdToFunction[0x28] = &GPU::VTX_DIFF;
    cmdToFunction[0x29] = &GPU::POLYGON_ATTR;
    cmdToFunction[0x2A] = &GPU::TEXIMAGE_PARAM;
    cmdToFunction[0x2B] = &GPU::PLTT_BASE;
    cmdToFunction[0x30] = &GPU::DIF_AMB;
    cmdToFunction[0x31] = &GPU::SPE_EMI;
    cmdToFunction[0x32] = &GPU::LIGHT_VECTOR;
    cmdToFunction[0x33] = &GPU::LIGHT_COLOR;
    cmdToFunction[0x34] = &GPU::SHININESS;
    cmdToFunction[0x40] = &GPU::BEGIN_VTXS;
    cmdToFunction[0x41] = &GPU::END_VTXS;
    cmdToFunction[0x50] = &GPU::SWAP_BUFFERS;
    cmdToFunction[0x60] = &GPU::VIEWPORT;
    cmdToFunction[0x70] = &GPU::BOX_TEST;
    cmdToFunction[0x71] = &GPU::POS_TEST;
    cmdToFunction[0x72] = &GPU::VEC_TEST;


}

void nds::GPU::writeRegister(Word addr, Byte val)
{
    if(addr == 0x04000060){
        DISP3DCNT.raw = (DISP3DCNT.raw & 0xFF00) | (val);
    }
    else if(addr == 0x04000061){
        DISP3DCNT.raw = (DISP3DCNT.raw & 0x00FF) | (HalfWord(val) << 8);
    }

    else if(addr >= 0x04000330 && addr < 0x04000340){
        edgeColor[addr - 0x04000330] = val;
    }
    else if(addr == 0x04000340){
        alphaTestRef = val;
    }
    else if(addr >= 0x04000350 && addr < 0x04000354){
        const Word offset = addr - 0x04000350;
        clearColor = (clearColor & ~(0xFF << (8 * offset))) | (Word(val) << (8 * offset));
    }
    else if(addr >= 0x04000354 && addr < 0x04000356){
        const Word offset = addr - 0x04000354;
        clearDepth = (clearDepth & ~(0xFF << (8 * offset))) | (HalfWord(val) << (8 * offset));
    }
    else if(addr >= 0x04000356 && addr < 0x04000358){
        const Word offset = addr - 0x04000356;
        clrImageOffset = (clrImageOffset & ~(0xFF << (8 * offset))) | (HalfWord(val) << (8 * offset));
    }
    else if(addr >= 0x04000358 && addr < 0x0400035C){
        const Word offset = addr - 0x04000358;
        fogColor = (fogColor & ~(0xFF << (8 * offset))) | (Word(val) << (8 * offset));
    }
    else if(addr >= 0x0400035C && addr < 0x0400035E){
        const Word offset = addr - 0x0400035C;
        fogOffset = (fogOffset & ~(0xFF << (8 * offset))) | (HalfWord(val) << (8 * offset));
    }
    else if(addr >= 0x04000360 && addr < 0x04000380){
        const Word offset = addr - 0x04000360;
        fogTable[offset] = val;
    }
    else if(addr >= 0x04000380 && addr < 0x040003C0){
        const Word offset = addr - 0x04000380;
        toonTable[offset] = val;
    }

    else if(addr >= 0x04000610 && addr < 0x04000612){
        const Word offset = addr - 0x04000610;
        disp1DotDepth= (disp1DotDepth & ~(0xFF << (8 * offset))) | (HalfWord(val) << (8 * offset));
    }

    else if(addr == 0x04000601){
        GXSTAT.state.matrixStackOverUnderflow &= ~(val >> 7) & 1u; // ACK
    }
    else if(addr == 0x04000603){
        GXSTAT.state.commandFifoIRQ = val >> 6; 
        updateIF();
    }
}

Byte nds::GPU::readRegister(Word addr)
{
    if(addr >= 0x04000060 && addr < 0x04000062){
        return DISP3DCNT.raw >> ((addr - 0x04000060) * 8);
    }

    if(addr == 0x04000320) return 46; // rendered line count

    // Ram Count fehlt

    if(addr == 0x04000600){
        return 0;
    }
    if(addr == 0x04000601){
        return (matrixStack.coordinateSP & 0x1F) | (matrixStack.projectionSP << 5) | (GXSTAT.state.matrixStackOverUnderflow << 7);
    }
    if(addr == 0x04000602){ // Ausgabe ist noch falsch
        return commandFifoSize & 0xFF;
    }
    if(addr == 0x04000603){
        return ((commandFifoSize >> 8) & 1u) | ((commandFifoSize < 128) << 1) | (commandFifo.empty() << 2) |  (!(commandFifo.empty() || haltGeometry) << 3) | (GXSTAT.raw >> 24);
    }
    // if(addr >= 0x04000600 && addr < 0x04000604){ // Nein, nicht so
    //     return GXSTAT.raw >> ((addr - 0x04000600) * 8);
    // }

    if(addr >= 0x04000620 && addr < 0x04000630){
        const Word offset = addr - 0x04000620;
        return posResult[offset];
    }
    if(addr >= 0x04000630 && addr < 0x04000636){
        const Word offset = addr - 0x04000630;
        return vecResult[offset];
    }
    if(addr >= 0x04000640 && addr < 0x04000680){
        const Word offset = addr - 0x04000640;
        return *(((Byte*)(&clipMtxResult.data[0])) + offset);
    }
    if(addr >= 0x04000680 && addr < 0x040006A4){
        const Word offset = addr - 0x04000680;
        return *(((Byte*)(&vecMtxResult.data[0])) + offset); // AKTUELL FALSCH
    }


    return 0;
}

// Hier sind wahrscheinlich noch Fehler
void nds::GPU::writeCommandRegister(Word addr, Word val)
{
    const Word index = addr - 0x04000400;
    const GXCMD cmd = addrToCmd[index];
    // buildingCommand.cmd = cmd;
    const Word params = cmdToNumParams[cmd];
    if(regParamsRemaining > 0){
        regParamsRemaining--;
        buildingRegCommand.params[paramRegIndex++] = val;
        // std::cout << "REG: Wrote param: " << getHex0x(val, 8) << "\n";
        if(regParamsRemaining<=0){
            // std::cout << "REG: finished command\n";
            commandFifo.push(buildingRegCommand);
            commandFifoSize += std::max(1u, params);
        }
    }
    else{
        buildingRegCommand.cmd = cmd;
        // std::cout << "REG: Wrote command " << getHex0x(cmd, 2) <<", param: " << getHex0x(val, 8) << "\n";
        regParamsRemaining = params;
        paramRegIndex = 0;
        buildingRegCommand.params[paramRegIndex++] = val;
        regParamsRemaining--;
        if(regParamsRemaining <= 0){
            // std::cout << "REG: finished command early\n";
            commandFifo.push(buildingRegCommand);
            commandFifoSize += std::max(1u, params);
        }
    }
}

void nds::GPU::writeGXFIFO(Word val)
{

    const auto pushZeroParamCommands = [&](){
        while(paramsRemaining <= 0 && gxFifo != 0){
            commandFifo.push(buildingCommand);
            commandFifoSize++;
            gxFifo >>= 8;
            buildingCommand.cmd = GXCMD(gxFifo & 0xFF);
            paramsRemaining = (int)cmdToNumParams[buildingCommand.cmd];
        }
    };

    if(gxFifo == 0){ // Fall: neuer Write
        // std::cout << "Wrote gxfifo command(s) " << getHex0x(val, 8) << "\n";
        gxFifo = val;
        buildingCommand.cmd = GXCMD(gxFifo & 0xFF);
        paramsRemaining = (int)cmdToNumParams[buildingCommand.cmd];
        paramIndex = 0;
        pushZeroParamCommands();
    }
    else{ // Fall: Commands in Pipe
        // std::cout << "gxfifo param for cmd: " << getHex0x(gxFifo & 0xFF, 2) << ", p: " << getHex0x(val, 8) << "\n";
        paramsRemaining--;
        buildingCommand.params[paramIndex++] = val;
        if(paramsRemaining<=0){
            commandFifo.push(buildingCommand);
            commandFifoSize += paramIndex;
            paramIndex = 0;
            gxFifo >>= 8; // alten Command rausshiften
            if(gxFifo != 0){ // noch mehr Commands vorhanden, neuer Command wird gesetzt
                buildingCommand.cmd = GXCMD(gxFifo & 0xFF);
                paramsRemaining = (int)cmdToNumParams[buildingCommand.cmd];

                pushZeroParamCommands();
            }
            // std::cout << "finished gxfifo command\n";
        }
    }
}
