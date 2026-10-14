#include "bus.h"
#include <iostream>
#include "framework/stringlib.h"
#include <assert.h>

using namespace nds;

void nds::SharedBus::loadCartHLE(const std::vector<uint8_t> &bytes)
{
    if(bytes.size() == 0) return;
    gameCart = Cartridge(this, bytes);
    nds9romRomStart = *(Word*)(bytes.data() + 0x20);
    // nds9romRomStart &= ~0xFFF; // align 0x1000
    nds9romRamStart = *(Word*)(bytes.data() + 0x28);
    nds9romSize = *(Word*)(bytes.data() + 0x2C);

    nds7romRomStart = *(Word*)(bytes.data() + 0x30);
    nds7romRamStart = *(Word*)(bytes.data() + 0x38);
    nds7romSize = *(Word*)(bytes.data() + 0x3C);
    
    Word entrynds9 = *(Word*)(bytes.data() + 0x24);
    Word entrynds7 = *(Word*)(bytes.data() + 0x34);

    // std::cout << "Start in " << getHex0x(nds9romRomStart + (entrynds9 - nds9romRamStart), 8) << "\n";
    
    bus7->cpu.setPC(entrynds7);
    bus7->cpu.setLR(entrynds7);
    bus7->cpu.setIP(entrynds7);
    bus9->cpu.setPC(entrynds9);
    bus9->cpu.setLR(entrynds9);
    bus9->cpu.setIP(entrynds9);

    // Werte von gbatek
    // bus7->writeByte(0x04000300, 1);
    // bus9->writeByte(0x04000300, 1);
    bus9->writeByte(0x04000247, 0x03);
    bus9->writeWord(0x027FF800, 0x00001FC2);
    bus9->writeWord(0x027FF804, 0x00001FC2);
    bus9->writeHalfWord(0x027FF850, 0x5835);
    bus9->writeHalfWord(0x027FF880, 0x0007);
    bus9->writeHalfWord(0x027FF884, 0x0006);
    bus9->writeWord(0x027FFC00, 0x00001FC2);
    bus9->writeWord(0x027FFC04, 0x00001FC2);
    bus9->writeHalfWord(0x027FFC10, 0x5835);
    bus9->writeHalfWord(0x027FFC40, 0x0001);

    // Generate cart ID
// 1st byte - Manufacturer (eg. C2h=Macronix) (roughly based on JEDEC IDs)
// 2nd byte - Chip size (00h..7Fh: (N+1)Mbytes, F0h..FFh: (100h-N)*256Mbytes?)
// 3rd byte - Flags
//   0   Maybe Infrared flag? (in case ROM does contain on-chip infrared stuff)
//   1   Unknown (set in some 3DS carts)
//   2-7 Zero
// 4th byte - Flags
//   0-2 Zero
//   3   Seems to be NAND flag (0=ROM, 1=NAND) (observed in only ONE cartridge)
//   4   3DS Flag (0=NDS/DSi, 1=3DS)
//   5   Zero   ... set in ... DSi-exclusive games? (support cmd B5h/D6h ?)
//   6   DSi flag (0=NDS/3DS, 1=DSi)
//   7   Cart Protocol Variant (0=older/smaller carts, 1=newer/bigger carts)
    // chipID = 0xC2; // Manufacturer = Macronix
    // if (bytes.size() >= 1024 * 1024 && bytes.size() <= 128 * 1024 * 1024) {
    //     chipID |= ((bytes.size() >> 20) - 1) << 8;
    // } else {
    //     chipID |= (0x100 - (bytes.size() >> 28)) << 8;
    // }

    bus9->writeWord(0x027FF800, chipID);
    bus9->writeWord(0x027FF804, chipID);
    bus9->writeWord(0x027FFC00, chipID);
    bus9->writeWord(0x027FFC04, chipID);

    // copy header
    for(size_t i = 0; i < 0x170; i++){
        mainMemory[0x3FFE00 + i] = bytes[i];
    }
    
    size_t j = nds9romRamStart;
    for(size_t i = nds9romRomStart; i < nds9romRomStart + nds9romSize; i++){
        bus9->writeByte(j, bytes[i]);
        j++;
    }

    j = nds7romRamStart;
    for(size_t i = nds7romRomStart; i < nds7romRomStart + nds7romSize; i++){
        bus7->writeByte(j, bytes[i]);
        j++;
    }
}

Byte nds::SharedBus::readByte(Word addr)
{
    if(addr >= 0x02000000 && addr < 0x03000000){
        // main mem
        addr -= 0x02000000;
        return mainMemory[addr % 0x40'0000];
    }
    return 0;
}

void nds::SharedBus::writeByte(Word addr, Byte val)
{
    if(addr >= 0x02000000 && addr < 0x03000000){
        // main mem
        addr -= 0x02000000;
        mainMemory[addr % 0x40'0000] = val;
    }
}

bool nds::SharedBus::canAccessGameCart(const bool isNDS9) const
{
    const bool rightsArm7 = ((EXMEMCNT >> 11) & 1u) > 0;
    return rightsArm7 ^ isNDS9;
}

void nds::SharedBus::PPUEnteredHBlank()
{
    bus7->PPUEnteredHBlank();
    bus9->PPUEnteredHBlank();
}

void nds::SharedBus::PPUEnteredVBlank()
{
    bus7->PPUEnteredVBlank();
    bus9->PPUEnteredVBlank();
}

void nds::SharedBus::PPULeftHBlank()
{
}

void nds::SharedBus::PPULeftVBlank()
{
}

void SharedBus::init()
{
    bus7 = std::make_shared<Bus7>(this);
    bus7->init();
    bus9 = std::make_shared<Bus9>(this);
    bus9->init();
    lcd = std::make_shared<LCD>(this);
    lcd->init();
    ipc = std::make_shared<IPC>(this);
    scheduler = std::make_shared<gba::Scheduler<EventType, SharedBus>>(this);
    scheduler->init();
}

void nds::SharedBus::setIF(int bit, bool val)
{
    bus7->setIF(bit, val);   
    bus9->setIF(bit, val);
}

void nds::SharedBus::setIFSingleBus(int bit, bool val, bool nds9)
{
    if(nds9){
        bus9->setIF(bit, val);
    }
    else{
        bus7->setIF(bit, val);
    }
}

void nds::SharedBus::setHalt()
{
    halted = true;
}

uint32_t *nds::SharedBus::accessFramebuffer()
{
    return lcd->accessFramebuffer();
}

std::pair<float, float> nds::SharedBus::getSample()
{
    return {0,0};
}

void nds::SharedBus::clock()
{
    // if(watchBreakpoints){
    //     if(std::find(breakpoints.begin(), breakpoints.end(), cpu.getRegisterState().R[15] - (cpu.state() == ARM ? 8 : 4)) != breakpoints.end()){
    //         halted = true;
    //     }
    //     if(std::find(breakpointsOP.begin(), breakpointsOP.end(), cpu.getCurrentOpcode()) != breakpointsOP.end()){
    //         halted = true;
    //     }
    // }

    if(!halted || steps > 0){
        bus7->clock();
        bus9->clock();
        scheduler->clock();

        if(steps > 0)
            steps--;
    }

}

void nds::SharedBus::press(int i){
    KEYINPUT.raw &= ~(1u << i);
    if(KEYCNT.raw & (1u << 14)){ // IRQ enable
        if(KEYCNT.raw & (1u << i)){
            if(KEYCNT.raw & (1u << 15)){ // AND
                if((KEYINPUT.raw & 0b1111111111u) == 0){
                    setIF(12, true);
                }
            }
            else{
                setIF(12, true);
            }
        }
    }
}

void nds::SharedBus::release(int i){
    KEYINPUT.raw |= (1u << i);
}
