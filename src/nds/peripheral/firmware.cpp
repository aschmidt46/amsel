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
                    status |= 1u;
                    std::cout << "FIRMWARE WRITE ENABLE\n";
                    return;
                }
                case 0x04:{ // write disable
                    status &= ~1u;
                    std::cout << "FIRMWARE WRITE DISABLE\n";
                    return;
                }
                case 0x9F:{ // read ID
                    state = FIRMWARE_READ_ID;
                    readWriteIndex = 0;
                    std::cout << "FIRMWARE READ ID\n";
                    return;
                }
                case 0x05:{ // read Status
                    state = FIRMWARE_READ_STATUS;
                    std::cout << "FIRMWARE READ STATUS\n";
                    return;
                }
                case 0x03:{ // read data
                    state = FIRMWARE_READ_DATA_ADDRESS;
                    readWriteIndex = 0;
                    address = 0;
                    std::cout << "FIRMWARE READ DATA ADDRESS\n";
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
                state = FIRMWARE_READ_DATA;
                readWriteIndex = address;
                std::cout << "FIRMWARE READ DATA AT ADDRESS: " << getHex0x(address, 6) << "\n";
                address = 0;
            }
            return;
        }
        case FIRMWARE_READ_STATUS:{
            state = FIRMWARE_IDLE;
            return;
        }
        default: return;
    }
}

Byte nds::Firmware::read()
{
    switch(state){
        case FIRMWARE_READ_ID:{
            const Byte current = ID[readWriteIndex];
            readWriteIndex++;
            if(readWriteIndex >= 3) state = FIRMWARE_IDLE;
            return current;
        }
        case FIRMWARE_READ_STATUS:{
            return status;
        }
        case FIRMWARE_READ_DATA:{
            const Byte current = firmwareData[readWriteIndex];
            readWriteIndex++;
            return current;
        }
        default: break;
    }
    return 0;
}
