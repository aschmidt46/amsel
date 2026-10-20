#include "eeprom.h"
#include <iostream>
#include "framework/stringlib.h"

using namespace nds;

bool nds::EEPROM::isWriteProtected(Word addr)
{
    auto p = (status >> 2) & 0b11;
    if(p == 1){
        if(addr >= 3 * (size / 4)) return true;
    }
    if(p == 2){
        if(addr >= size / 2) return true;
    }
    if(p == 3) return true;
    return false;
}

nds::EEPROM::EEPROM(size_t size) : size(size)
    , addressLength(size >= 0x20000 ? 3 : (size >= 0x2000 ? 2 : 1))
    , pageSize(size >= 0x20000 ? 0x20000 : (size >= 0x10000 ? 128 : (size >= 0x2000 ? 32 : 16)))
{
    data = std::vector<Byte>(size, 0);
}

void nds::EEPROM::loadSave(const std::vector<Byte> &data){
    this->data = data;
}

std::vector<Byte> nds::EEPROM::getSave()
{
    return data;
}

// For all EEPROM and FRAM types:
// 
//   06h WREN  Write Enable                Cmd, no parameters
//   04h WRDI  Write Disable               Cmd, no parameters
//   05h RDSR  Read Status Register        Cmd, read repeated status value(s)
//   01h WRSR  Write Status Register       Cmd, write one-byte value
//   9Fh RDID  Read JEDEC ID (not supported on EEPROM/FLASH, returns FFh-bytes)
// 
// For 0.5K EEPROM (8+1bit Address):
// 
//   03h RDLO  Read from Memory 000h-0FFh  Cmd, addr lsb, read byte(s)
//   0Bh RDHI  Read from Memory 100h-1FFh  Cmd, addr lsb, read byte(s)
//   02h WRLO  Write to Memory 000h-0FFh   Cmd, addr lsb, write 1..MAX byte(s)
//   0Ah WRHI  Write to Memory 100h-1FFh   Cmd, addr lsb, write 1..MAX byte(s)
// 
// For 8K..64K EEPROM and for FRAM (16bit Address):
// 
//   03h RD    Read from Memory            Cmd, addr msb,lsb, read byte(s)
//   02h WR    Write to Memory             Cmd, addr msb,lsb, write 1..MAX byte(s)
// 
// For 128K EEPROM (24bit Address):
// 
//   As above, but with 24bit addr msb,mid,lsb ?
// 

void nds::EEPROM::write(Byte val)
{
    switch(state){
        case EEPROM_STATE_IDLE:{
            switch(val){
                case 0x02:{
                    state = EEPROM_STATE_WRITE_ADDRESS;
                    remainingInput = addressLength;
                    runningAddress = 0;
                    break;
                }
                case 0x0A:{
                    state = EEPROM_STATE_WRITE_ADDRESS;
                    remainingInput = addressLength;
                    runningAddress = 1u; // write "high", aka 0x100-0x1FF, 1 Byte Adresse
                    break;
                }
                case 0x03:{
                    state = EEPROM_STATE_READ_ADDRESS;
                    remainingInput = addressLength;
                    runningAddress = 0;
                    break;
                }
                case 0x0B:{
                    state = EEPROM_STATE_READ_ADDRESS;
                    remainingInput = addressLength;
                    runningAddress = 1u; // write "high", aka 0x100-0x1FF, 1 Byte Adresse
                    break;
                }
                case 0x06:{
                    std::cout << "EEPROM WRITE ENABLE\n";
                    status |= 2;
                    break;
                }
                case 0x04:{
                    std::cout << "EEPROM WRITE DISABLE\n";
                    status &= ~2;
                    break;
                }
                case 0x05:{
                    std::cout << "EEPROM READ STATUS\n";
                    state = EEPROM_STATE_READ_STATUS;
                    break;
                }
                case 0x01:{
                    std::cout << "EEPROM WRITE STATUS\n";
                    state = EEPROM_STATE_WRITE_STATUS;
                    break;
                }
                case 0x9F:{
                    std::cout << "EEPROM READ JEDEC\n";
                    state = EEPROM_STATE_READ_JEDEC;
                    break;
                }
                default:{
                    std::cout << "EEPROM UNKNOWN?: " << getHex0x(val, 2) << "\n";
                    break;
                }
            }
            break;
        }
        case EEPROM_STATE_READ_ADDRESS:
        case EEPROM_STATE_WRITE_ADDRESS:{
            runningAddress <<= 8;
            runningAddress |= val;
            remainingInput--;
            if(remainingInput <= 0){
                if(state == EEPROM_STATE_READ_ADDRESS){
                    std::cout << "EEPROM READ AT: ";
                }
                else std::cout << "EEPROM WRITE AT: ";
                std::cout << getHex0x(runningAddress, 6) << "\n";
                state = (state == EEPROM_STATE_READ_ADDRESS) ? EEPROM_STATE_READ : EEPROM_STATE_WRITE;
                remainingPage = pageSize;
            }
            break;
        }
        case EEPROM_STATE_WRITE:{
            if(!isWriteProtected(runningAddress) && (status & 2u) && runningAddress < this->data.size() && remainingPage > 0){ // kein write disable auf adresse
                this->data[runningAddress] = val;
                remainingPage--;
                std::cout << "WRITE DATA: " << getHex0x(data[runningAddress], 2) << "\n";
                runningAddress++;
            }
            break;
        }
        case EEPROM_STATE_WRITE_STATUS:{
            val &= 0b10001100;
            if(size < 0x2000) val &= ~(1u << 7);
            status = val;
            state = EEPROM_STATE_IDLE;
            break;
        }
        default:{
            break;
        }
    }
}

Byte nds::EEPROM::read()
{
    switch(state){
        case EEPROM_STATE_READ_JEDEC:{
            return 0xFF;
        }
        case EEPROM_STATE_READ_STATUS:{
            Byte tmp = status;
            if(size == 0x200) tmp |= (0b1111 << 4);
            return tmp;
        }
        case EEPROM_STATE_READ:{
            if(runningAddress < data.size()){
                std::cout << "READ DATA: " << getHex0x(data[runningAddress], 2) << "\n";
                return data[runningAddress++];
            }
            return 0;
        }
        default:{
            return 0;
        }
    }
}
