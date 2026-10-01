#include "rtc.h"
#include <chrono>
#include <ctime>
#include "../bus.h"

using namespace std::chrono;
using std::chrono::system_clock;

using namespace gba;

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

void gba::GPIO::onWrite(Word addr, Byte val)
{
    if(addr == 0){
        data = val & 0xF;
    }
    else if(addr == 1){
        direction = val & 0xF;
    }
    else if(addr == 2){
        control = val & 1u;   
    }

}

Byte gba::GPIO::onRead(Word addr)
{
    if(control & 1u){
        if(addr == 0){
            return data & ~direction; // direction 0: in
        }
        else if(addr == 1){
            return direction;
        }
        else if(addr ==2){
            return control;
        }
    }
    return 0;
}

void gba::RTC::putOnReadQueue()
{
    registerQueue = std::vector<Byte>(0);

    auto tp = zoned_time{current_zone(), system_clock::now()}.get_local_time();
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
    if(!rtcControl.state.mode24h){
        h %= 12;
    }
    auto M = time.minutes().count();
    auto s = time.seconds().count();
    switch(registerIndexSelected){
        case 1:{
            registerQueue.push_back(rtcControl.raw & rtcControlMask);
            rtcControl.state.powerOff = 0;
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
            break;
        }
        case 3:{
            registerQueue.push_back(dec2bcd(h));
            registerQueue.push_back(dec2bcd(M));
            registerQueue.push_back(dec2bcd(s));
            break;
        }
        case 0:{
            rtcControl.raw = 0;
            mode = RTC_IDLE;
            break;
        }
        case 6:{
            // GamePak interrupt
            bus->setIF(13, true);
            mode = RTC_IDLE;
            break;
        }
        default:{
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

void gba::RTC::onWrite(Word addr, Byte val)
{
    GPIO::onWrite(addr, val);
    bool prevSCK = SCK;
    SCK = (data & 1) > 0;
    SIO = (data & 2) > 0;
    CS = (data & 4) > 0;

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

    if(!prevSCK && SCK){ // rising edge
        if(mode == RTC_CMD){
            readCommandLatch = (readCommandLatch << 1) | (SIO ? 1 : 0);
            bitsRead++;
            if(bitsRead >= 8){
                Byte test = (readCommandLatch >> 4) & 0b1111u;
                reverseBitOrder = test == 0b0110 ? false : true;
                if(reverseBitOrder) readCommandLatch = reverseByte(readCommandLatch);

                registerIndexSelected = (readCommandLatch >> 1) & 0b111;
                if(readCommandLatch & 1u){
                    mode = RTC_CMD_READ;
                    putOnReadQueue();
                }
                else{
                    mode = RTC_CMD_WRITE;
                    readCommandLatch = 0; // wird verwendet für Kontrollregister read
                }
                bitsRead = 0;
                bytesRead = 0;
            }
        }
        else if(mode == RTC_CMD_READ){
            data &= ~0b10;
            data |= (registerQueue[bytesRead] >> bitsRead) << 1;
            bitsRead++;
            if(bitsRead >= 8){
                bitsRead = 0;
                bytesRead++;
            }
            if(bytesRead >= registerQueue.size()){
                mode = RTC_IDLE;
            }
        }
        else if(mode == RTC_CMD_WRITE){
            if(registerIndexSelected == 1){ // control
                Byte bit = (data >> 1) & 1u;
                readCommandLatch <<= 1;
                readCommandLatch |= bit;
                bitsRead++;
                if(bitsRead >= 8){
                    rtcControl.raw = readCommandLatch & rtcControlMask & 0b01111111;
                    mode = RTC_IDLE;
                }
            }
        }
    }
}

Byte gba::RTC::onRead(Word addr)
{
    return GPIO::onRead(addr);
}
