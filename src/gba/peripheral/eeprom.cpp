#include "eeprom.h"
#include "../bus.h"
#include <iostream>
#include "framework/stringlib.h"

using namespace gba;

void gba::EEPROM::OnWrite(Byte val)
{
    val &= 1u;
    switch(state){
        case EEPROM_IDLE:{
            if(val == 1){
                state = EEPROM_READ_OR_WRITE;
            }
            break;
        }
        case EEPROM_READ_OR_WRITE:{
            if(val == 1){
                state = EEPROM_READ_ADDRESS;
                if(!initialized){
                    if(bus->dma[3].maxCount == 9){
                        setSize(0x40);
                    }
                    else if(bus->dma[3].maxCount == 17){
                        setSize(0x400);
                    }
                    else{
                        std::cout << "Unknown EEPROM Transfer Size: " << bus->dma[3].maxCount << "\n";
                    }
                }
                counter = memory.size() == 0x40 ? 6 : 14;
                address = 0;
            }
            else if(val == 0){
                state = EEPROM_WRITE_ADDRESS;
                if(!initialized){
                    if(bus->dma[3].maxCount == 9){
                        setSize(0x40);
                    }
                    else if(bus->dma[3].maxCount == 17){
                        setSize(0x400);
                    }
                    else{
                        std::cout << "Unknown EEPROM Transfer Size: " << bus->dma[3].maxCount << "\n";
                    }
                }
                counter = memory.size() == 0x40 ? 6 : 14;
                address = 0;
            }
            break;
        }
        case EEPROM_READ_ADDRESS:{
            if(counter > 0){
                address <<= 1;
                address |= val;
                counter--;
            }
            else{
                state = EEPROM_READ_FILLER;
                counter = 4;
            }
            break;
        }
        case EEPROM_WRITE_ADDRESS:{
            if(counter > 0){
                address <<= 1;
                address |= val;
                counter--;
            }
            if(counter == 0){
                state = EEPROM_WRITE;
                counter = 64;
            }
            break;
        }
        case EEPROM_WRITE:{
            if(counter == 0){
                // std::cout << address <<" address write, size: "<<memory.size()<<"\n";
                memory[address] = buffer;
                state = EEPROM_IDLE;
                buffer = 0;
                address = 0;
                // 0 write in IDLE ignoriert
            }
            if(counter > 0){
                // msb zuerst
                buffer <<= 1;
                buffer |= val;
                counter--;
            }
            break;
        }
        default:
            state = EEPROM_IDLE;
            break;
    }
}

Byte gba::EEPROM::OnRead()
{
    Byte res = 0;
    switch(state){
        case EEPROM_READ_FILLER:{
            if(counter > 0){
                counter--;
            }
            if(counter == 0){
                state = EEPROM_READ;
                // std::cout << getHex0x(address, 4) <<" address read, size: "<<memory.size()<<"\n";
                buffer = memory[address];
                counter = 64;
                outputctr = 0;
            }
            break;
        }
        case EEPROM_READ:{
            if(counter >= 0){
                // msb zuerst
                res = (buffer >> (counter-1)) & 1u;
                outputctr++;
                if(counter > 0){
                    counter--;
                }
            }
            if(counter == 0){
                state = EEPROM_IDLE;
                buffer = 0;
            }
            return res;
        }
        case EEPROM_IDLE:{
            return 1;
        }
        default:
            break;
    }
    return res;
}

void gba::EEPROM::loadSave(const std::vector<uint8_t> &saveData)
{
    memory = std::vector<uint64_t>(saveData.size() / 8);
    for(size_t i = 0; i < memory.size(); i++){
        for(size_t b = 0; b < 8; b++){
            memory[i] |= (uint64_t(saveData[8*i + b]) << (8 * b)) ;
        }
    }
    initialized = true;
}

std::vector<Byte> gba::EEPROM::getSaveData()
{
    std::vector<Byte> data(8 * memory.size());

    for(size_t i = 0; i < memory.size(); i++){
        for(size_t b = 0; b < 8; b++){
            data[8 * i + b] = memory[i] >> (8 * b);
        }
    }

    return data;
}

size_t gba::EEPROM::getSaveDataSize()
{
    return memory.size() * 8;
}

void gba::EEPROM::setSize(size_t size)
{
    memory = std::vector<uint64_t>(size);
    initialized = true;
}
