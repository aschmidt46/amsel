#include "flash.h"
#include <iostream>
#include "framework/stringlib.h"

using namespace nds;

void Flash::loadSave(const std::vector<Byte> &data){
    flashData = data;
}

std::vector<Byte> nds::Flash::getSave()
{
    return flashData;
}

void nds::Flash::write(Byte data)
{

    if(state == FLASH_DP){
        if(data == 0xAB){
            state = FLASH_IDLE;
        }
        else{
            return;
        }
    }

    switch(state){
        case FLASH_IDLE:{
            switch(data){
                case 0x06:{ // write enable
                    status |= 2u;
                    // std::cout << "FLASH WRITE ENABLE\n";
                    return;
                }
                case 0x04:{ // write disable
                    status &= ~2u;
                    // std::cout << "FLASH WRITE DISABLE\n";
                    return;
                }
                case 0x9F:{ // read ID
                    state = FLASH_READ_ID;
                    readWriteIndex = 0;
                    awaitRead = true;
                    // std::cout << "FLASH READ ID\n";
                    return;
                }
                case 0x05:{ // read Status
                    state = FLASH_READ_STATUS;
                    awaitRead = true;
                    // std::cout << "FLASH READ STATUS\n";
                    return;
                }
                case 0x0B:{
                    state = FLASH_READ_DATA_ADDRESS_FAST;
                    readWriteIndex = 0;
                    address = 0;
                    return;
                }
                case 0x03:{ // read data
                    state = FLASH_READ_DATA_ADDRESS;
                    readWriteIndex = 0;
                    address = 0;
                    // std::cout << "FLASH READ DATA ADDRESS\n";
                    return;
                }
                case 0x0A:{ // page write
                    state = FLASH_PW_ADDRESS;
                    readWriteIndex = 0;
                    address = 0;
                    // std::cout << "FLASH PAGE WRITE ADDRESS\n";
                    return;
                }
                case 0x02:{ // page program
                    state = FLASH_PP_ADDRESS;
                    readWriteIndex = 0;
                    address = 0;
                    // std::cout << "FLASH PAGE WRITE ADDRESS\n";
                    return;
                }
                case 0xDB:{ // page erase
                    state = FLASH_PE_ADDRESS;
                    readWriteIndex = 0;
                    address = 0;
                    // std::cout << "FLASH PAGE WRITE ADDRESS\n";
                    return;
                }
                case 0xD8:{ // page erase
                    state = FLASH_SE_ADDRESS;
                    readWriteIndex = 0;
                    address = 0;
                    // std::cout << "FLASH PAGE WRITE ADDRESS\n";
                    return;
                }
                case 0xB9:{
                    state = FLASH_DP;
                    return;
                }
                default:{
                    std::cout << "ERROR: UNKNOWN FLASH CMD: " << getHex0x(data, 2) << "\n";
                    return;
                }
            }
        }
        case FLASH_READ_DATA_ADDRESS_FAST:
        case FLASH_READ_DATA_ADDRESS:{
            address <<= 8;
            address |= data;
            readWriteIndex++;
            if(readWriteIndex >= 3){
                // std::cout << "Read Adress Selected: " << getHex0x(address, 6) << "\n";
                state = FLASH_READ_DATA;
                awaitRead = true;
                readWriteIndex = address;
                if(state == FLASH_READ_DATA_ADDRESS_FAST) readWriteIndex -= 1; // dummy byte
                // std::cout << "FLASH READ DATA AT ADDRESS: " << getHex0x(address, 6) << "\n";
                // if(address >= firmwareData.size()) std::cout << "FLASH READ OUT OF BOUNDS!\n";
                address = 0;
            }
            return;
        }
        case FLASH_SE_ADDRESS:
        case FLASH_PE_ADDRESS:
        case FLASH_PP_ADDRESS:
        case FLASH_PW_ADDRESS:{
            address <<= 8;
            address |= data;
            readWriteIndex++;
            if(readWriteIndex >= 3){
                // std::cout << "PW Selected: " << getHex0x(address, 6) << "\n";
                if(state == FLASH_PW_ADDRESS){
                    state = FLASH_PW;
                }
                else if(state == FLASH_PP_ADDRESS){
                    state  = FLASH_PP;
                }
                
                remainingBytes = 256;
                readWriteIndex = address;
                // Page Erase
                Word page = readWriteIndex / 256;
                origPage = page;
                if(state == FLASH_PE_ADDRESS){
                    for(Word i = page * 256; i < (page + 1) * 256; i++){
                        flashData[i] = 0xFF;
                    }
                    status = 0;
                    state = FLASH_IDLE;
                }
                else if(state == FLASH_SE_ADDRESS){
                    Word addr = readWriteIndex & ~0xFFFF; // align sector
                    for(Word i = addr; i < addr + 0x10000; i++){
                        if(i < flashData.size())
                            flashData[i] = 0xFF;
                    }
                    status = 0;
                    state = FLASH_IDLE;
                }

                // std::cout << "FLASH READ DATA AT ADDRESS: " << getHex0x(address, 6) << "\n";
                // if(address >= firmwareData.size()) std::cout << "FLASH READ OUT OF BOUNDS!\n";
                address = 0;
            }
            return;
        }
        case FLASH_PW:{
            Word page = readWriteIndex / 256;
            if(page == origPage){
                flashData[readWriteIndex] = data;
                // std::cout << "Wrote " << getHex0x(data, 2) << " at " << getHex0x(readWriteIndex, 6) << "\n";
            }
            else{
                // std::cout << "Cross page\n";
            }
            readWriteIndex++;
            remainingBytes--;
            if(remainingBytes <= 0){
                state = FLASH_IDLE;
                status &= ~2u;
                remainingBytes = 0;
            }
            return;
        }
        case FLASH_PP:{
            Word page = readWriteIndex / 256;
            if(page == origPage){
                flashData[readWriteIndex] = flashData[readWriteIndex] & ~data; // so?
                // std::cout << "Wrote " << getHex0x(data, 2) << " at " << getHex0x(readWriteIndex, 6) << "\n";
            }
            else{
                // std::cout << "Cross page\n";
            }
            readWriteIndex++;
            remainingBytes--;
            if(remainingBytes <= 0){
                state = FLASH_IDLE;
                status &= ~2u;
                remainingBytes = 0;
            }
            return;
        }
        case FLASH_READ_STATUS:{
            // state = FLASH_IDLE;
            return;
        }
        default: return;
    }
}

Byte nds::Flash::read()
{
    // besagter Hack
    if(awaitRead){
        awaitRead = false;
        return 0;
    }

    switch(state){
        case FLASH_READ_ID:{
            const Byte current = ID[readWriteIndex];
            readWriteIndex++;
            if(readWriteIndex >= 3) state = FLASH_IDLE;
            return current;
        }
        case FLASH_READ_STATUS:{
            // std::cout << "read status: " << int(status) << "\n";
            return status;
        }
        case FLASH_READ_DATA:{
            const Byte current = readWriteIndex < flashData.size() ? flashData[readWriteIndex] : 0;
            // std::cout << "read data firmware: " << getHex0x(current, 2) << "\n";
            readWriteIndex++;
            return current;
        }
        default: break;
    }
    return 0;
}
