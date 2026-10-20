#include "firmware.h"
#include <iostream>
#include "framework/stringlib.h"

using namespace nds;

void nds::Firmware::write(Byte data)
{
    switch(state){
        case FIRMWARE_IDLE:{
            switch(data){
                case 0x06:{ // write enable
                    status |= 2u;
                    // std::cout << "FIRMWARE WRITE ENABLE\n";
                    return;
                }
                case 0x04:{ // write disable
                    status &= ~2u;
                    // std::cout << "FIRMWARE WRITE DISABLE\n";
                    return;
                }
                case 0x9F:{ // read ID
                    state = FIRMWARE_READ_ID;
                    readWriteIndex = 0;
                    awaitRead = true;
                    // std::cout << "FIRMWARE READ ID\n";
                    return;
                }
                case 0x05:{ // read Status
                    state = FIRMWARE_READ_STATUS;
                    awaitRead = true;
                    // std::cout << "FIRMWARE READ STATUS\n";
                    return;
                }
                case 0x03:{ // read data
                    state = FIRMWARE_READ_DATA_ADDRESS;
                    readWriteIndex = 0;
                    address = 0;
                    // std::cout << "FIRMWARE READ DATA ADDRESS\n";
                    return;
                }
                case 0x0A:{ // page write
                    state = FIRMWARE_PW_ADDRESS;
                    readWriteIndex = 0;
                    address = 0;
                    // std::cout << "FIRMWARE PAGE WRITE ADDRESS\n";
                    return;
                }
                default:{
                    std::cout << "ERROR: UNKNOWN FIRMWARE CMD: " << getHex0x(data, 2) << "\n";
                    return;
                }
            }
        }
        case FIRMWARE_READ_DATA_ADDRESS:{
            address <<= 8;
            address |= data;
            readWriteIndex++;
            if(readWriteIndex >= 3){
                // std::cout << "Read Adress Selected: " << getHex0x(address, 6) << "\n";
                state = FIRMWARE_READ_DATA;
                awaitRead = true;
                readWriteIndex = address;
                // std::cout << "FIRMWARE READ DATA AT ADDRESS: " << getHex0x(address, 6) << "\n";
                // if(address >= firmwareData.size()) std::cout << "FIRMWARE READ OUT OF BOUNDS!\n";
                address = 0;
            }
            return;
        }
        case FIRMWARE_PW_ADDRESS:{
            address <<= 8;
            address |= data;
            readWriteIndex++;
            if(readWriteIndex >= 3){
                // std::cout << "PW Selected: " << getHex0x(address, 6) << "\n";
                state = FIRMWARE_PW;
                remainingBytes = 256;
                readWriteIndex = address;
                // Page Erase
                Word page = readWriteIndex / 256;
                origPage = page;
                // Page Erase findet nicht transparent statt, weil Flash dann intern alte Werte zurückschreibt?
                // for(Word i = page * 256; i < page * 256 + 256; i++){
                //     std::cout << getHex0x(i, 6) << " cleared\n";
                //     firmwareData[i] = 0;
                // }

                // std::cout << "FIRMWARE READ DATA AT ADDRESS: " << getHex0x(address, 6) << "\n";
                // if(address >= firmwareData.size()) std::cout << "FIRMWARE READ OUT OF BOUNDS!\n";
                address = 0;
            }
            return;
        }
        case FIRMWARE_PW:{
            Word page = readWriteIndex / 256;
            if(page == origPage){
                firmwareData[readWriteIndex] = data;
                // std::cout << "Wrote " << getHex0x(data, 2) << " at " << getHex0x(readWriteIndex, 6) << "\n";
            }
            else{
                // std::cout << "Cross page\n";
            }
            readWriteIndex++;
            remainingBytes--;
            if(remainingBytes <= 0){
                state = FIRMWARE_IDLE;
                status &= ~2u;
                remainingBytes = 0;
            }
            return;
        }
        case FIRMWARE_READ_STATUS:{
            // state = FIRMWARE_IDLE;
            return;
        }
        default: return;
    }
}

Byte nds::Firmware::read()
{
    // besagter Hack
    if(awaitRead){
        awaitRead = false;
        return 0;
    }

    switch(state){
        case FIRMWARE_READ_ID:{
            const Byte current = ID[readWriteIndex];
            readWriteIndex++;
            if(readWriteIndex >= 3) state = FIRMWARE_IDLE;
            return current;
        }
        case FIRMWARE_READ_STATUS:{
            // std::cout << "read status: " << int(status) << "\n";
            return status;
        }
        case FIRMWARE_READ_DATA:{
            const Byte current = readWriteIndex < firmwareData.size() ? firmwareData[readWriteIndex] : 0;
            // std::cout << "read data firmware: " << getHex0x(current, 2) << "\n";
            readWriteIndex++;
            return current;
        }
        default: break;
    }
    return 0;
}
