#include "rtc.h"
#include <chrono>
#include <ctime>
#include "../bus7.h"
#include <iostream>
#include <bitset>
#include "framework/stringlib.h"

using namespace std::chrono;
using std::chrono::system_clock;

using namespace nds;

namespace nds{
    Byte reverseByte(Byte b) {
       b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
       b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
       b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
       return b;
    }
    
    Byte dec2bcd(Byte dec) 
    {
        Byte result = 0;
        int shift = 0;
    
        while (dec)
        {
            result +=  (dec % 10) << shift;
            dec = dec / 10;
            shift += 4;
        }
        return result;
    }
}


void nds::GPIO::onWrite(Word addr, Byte val)
{

    if(addr == 0x4000138){
        reg.raw = (reg.raw & (1u) & ~(reg.state.dataDir)) | (val & (0b01110110)) | (val & 1u & (reg.state.dataDir));
    }

}

Byte nds::GPIO::onRead(Word addr)
{
    if(addr == 0x4000138){
        // std::cout << "RTC READ: " << std::bitset<8>((reg.raw & ~(1u)) | (reg.raw & (1u) & ~(reg.state.dataDir))) << "\n";
        return (reg.raw & ~(1u)) | (reg.raw & (1u) & ~(reg.state.dataDir)); // direction 0: in
    }   
    return 0;
}

void nds::RTC::putOnReadQueue()
{
    registerQueue = std::vector<Byte>(0);

    // #ifdef BUILD_DESKTOP
    // auto tp = std::chrono::zoned_time{current_zone(), system_clock::now()}.get_local_time();
    // #else
    auto tp = system_clock::now();
    // #endif
    auto dp = floor<days>(tp);
    year_month_day ymd{dp};
    hh_mm_ss time{floor<milliseconds>(tp-dp)};
    std::chrono::weekday wd{dp};
    unsigned int weekday = wd.c_encoding();
    auto y = ymd.year();
    unsigned int y2000 = static_cast<int>(y) - 2000;
    auto m = ymd.month();
    auto d = ymd.day();
    auto h = time.hours().count();
    if(!rtcStat1.state.mode24h){
        h %= 12;
    }
    auto M = time.minutes().count();
    auto s = time.seconds().count();
    switch(registerIndexSelected){
        case 1:{
            registerQueue.push_back(rtcStat2.raw);
            // std::cout << "read reg1 (stat2)\n";
            break;
        }
        case 2:{
            registerQueue.push_back(dec2bcd(y2000));
            registerQueue.push_back(dec2bcd(static_cast<unsigned>(m)));
            registerQueue.push_back(dec2bcd(static_cast<unsigned>(d)));
            registerQueue.push_back(weekday);

            registerQueue.push_back(dec2bcd(h));
            registerQueue.push_back(dec2bcd(M));
            registerQueue.push_back(dec2bcd(s));
            // std::cout << "read DateTime\n";
            break;
        }
        case 3:{
            registerQueue.push_back(dec2bcd(h));
            registerQueue.push_back(dec2bcd(M));
            registerQueue.push_back(dec2bcd(s));
            // std::cout << "read Time\n";
            break;
        }
        case 0:{
            registerQueue.push_back(rtcStat1.raw & rtcStat1Mask);
            // std::cout << "read reg 0 (stat1)\n";
            break;
        }
        case 6:{
            // GamePak interrupt
            // bus->setIF(7, true);
            // mode = RTC_IDLE;
            registerQueue.push_back(clkAdjust);
            // std::cout << "read reg 6\n";
            break;
        }
        case 7:{
            // std::cout << "read reg free\n";
            registerQueue.push_back(freereg);
            break;
        }
        default:{
            // std::cout << "read reg ?\n";
            mode = RTC_IDLE;
            break;
        }
    }
    if(reverseBitOrder){
        for(auto &b : registerQueue){
            b = reverseByte(b);
        }
    }
}

void nds::RTC::onWrite(Word addr, Byte val)
{
    GPIO::onWrite(addr, val);
    bool prevSCK = SCK;
    SCK = reg.state.clock > 0; // clock inverted also == 0
    SIO = reg.state.dataio > 0;
    CS = reg.state.cs > 0;

    if(mode == RTC_IDLE && SCK && CS){ // SCK und CS -> Start CMD
        mode = RTC_CMD;
        bitsRead = 0;
        bytesRead = 0;
        readCommandLatch = 0;
        return;
    }
    if(!CS){ // !CS schließt ab
        mode = RTC_IDLE;
        return;
    }

    if(prevSCK && !SCK){ // falling edge
        if(mode == RTC_CMD){
            readCommandLatch = (readCommandLatch << 1) | (SIO ? 1 : 0);
            bitsRead++;
            if(bitsRead == 8){
                // std::cout << "command: " << std::bitset<8>(readCommandLatch) << "\n";
                Byte test = (readCommandLatch >> 4) & 0b1111u;
                reverseBitOrder = test == 0b0110 ? false : true;
                if(reverseBitOrder){
                    readCommandLatch = reverseByte(readCommandLatch);
                }

                registerIndexSelected = (readCommandLatch >> 1) & 0b111;
                if(readCommandLatch & 1u){
                    // std::cout << "mode: read\n";
                    mode = RTC_CMD_READ;
                    putOnReadQueue();
                }
                else{
                    // std::cout << "mode: write\n";
                    mode = RTC_CMD_WRITE;
                    readCommandLatch = 0; // wird verwendet für Kontrollregister read
                }
                bitsRead = 0;
                bytesRead = 0;
            }
        }
        else if(mode == RTC_CMD_READ){
            reg.state.dataio = (registerQueue[bytesRead] >> bitsRead) & 1u;
            bitsRead++;
            if(bitsRead >= 8){
                // std::cout << "output 1 byte\n";
                bitsRead = 0;
                bytesRead++;
            }
            if(bytesRead >= registerQueue.size()){
                mode = RTC_IDLE;
            }
        }
        else if(mode == RTC_CMD_WRITE){
            if(registerIndexSelected == 1){ // stat2
                Byte bit = reg.state.dataio;
                readCommandLatch <<= 1;
                readCommandLatch |= bit;
                bitsRead++;
                if(bitsRead >= 8){
                    rtcStat2.raw = readCommandLatch;
                    mode = RTC_IDLE;
                }
            }
            if(registerIndexSelected == 0){ // stat1
                Byte bit = reg.state.dataio;
                readCommandLatch <<= 1;
                readCommandLatch |= bit;
                bitsRead++;
                if(bitsRead >= 8){
                    rtcStat1.raw = readCommandLatch & rtcStat1Mask;
                    mode = RTC_IDLE;
                }
            }
            if(registerIndexSelected == 6){ // clkadjust
                Byte bit = reg.state.dataio;
                readCommandLatch <<= 1;
                readCommandLatch |= bit;
                bitsRead++;
                if(bitsRead >= 8){
                    clkAdjust = readCommandLatch;
                    mode = RTC_IDLE;
                }
            }
            if(registerIndexSelected == 7){ // free
                Byte bit = reg.state.dataio;
                readCommandLatch <<= 1;
                readCommandLatch |= bit;
                bitsRead++;
                if(bitsRead >= 8){
                    freereg = readCommandLatch;
                    mode = RTC_IDLE;
                }
            }
        }
    }
}

Byte nds::RTC::onRead(Word addr)
{
    return GPIO::onRead(addr);
}
