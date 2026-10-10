#include "maths.h"
#include <bit>
#include <cstdint>
#include <limits>
#include <iostream>
#include <cmath>

using namespace nds;

void nds::DSMaths::startDivision()
{
    const Word mode = (DIVCNT & 0b11);
    if(mode == 0){
        remainingDiv = 18;
        DIVCNT |= 1u << 15;
    }
    else if (mode < 3){
        remainingDiv = 34;
        DIVCNT |= 1u << 15;
    }

    
    
    // illegal
    else{
        remainingDiv = -1;
        DIVCNT &= ~(1u << 15);
    }
}

void nds::DSMaths::startSqrt()
{
    remainingSqrt = 13;
    SQRTCNT |= 1u << 15;
}

void nds::DSMaths::doDivision()
{
    const Word mode = (DIVCNT & 0b11);

    DIVCNT &= ~(1u << 15);
    if(divDenom == 0){
        DIVCNT |= 1u << 14;
    }
    else DIVCNT &= ~(1u << 14);

    switch(mode){
        case 0:{
            int32_t num = std::bit_cast<int32_t>(uint32_t(divNumer));
            int32_t denom = std::bit_cast<int32_t>(uint32_t(divDenom));
            if(denom == 0){
                divRemResult = std::bit_cast<uint32_t>(num);
                divResult = num >= 0 ? std::bit_cast<uint32_t>(-1) : 1;
                if(num < 0){
                    divRemResult = sign_extend_n_64(divRemResult, 32);
                    divResult |= uint64_t(std::bit_cast<uint32_t>(-1)) << 32;
                }
                break;
            }
            else if(denom == -1 && num == std::numeric_limits<int32_t>::min()){
                divResult = std::bit_cast<uint32_t>(num);
                divRemResult = 0;
                break;
            }
            int32_t result = num / denom;
            int32_t m = num % denom;
            divResult = sign_extend_n_64(std::bit_cast<uint32_t>(result), 32);
            divRemResult = sign_extend_n_64(std::bit_cast<uint32_t>(m), 32);
            break;
        }
        case 1:{
            int64_t num = std::bit_cast<int64_t>(divNumer);
            int32_t denom = std::bit_cast<int32_t>(uint32_t(divDenom));
            if(denom == 0){
                divRemResult = std::bit_cast<uint64_t>(num);
                divResult = num >= 0 ? std::bit_cast<uint64_t>((int64_t)-1) : 1;
                break;
            }
            else if(denom == -1 && num < std::numeric_limits<int32_t>::min()){
                divResult = std::bit_cast<uint64_t>(num);
                divRemResult = 0;
                break;
            }
            int64_t result = num / denom;
            int32_t m = num % denom;
            divResult = std::bit_cast<uint64_t>(result);
            divRemResult = sign_extend_n_64(std::bit_cast<uint32_t>(m), 32);
            break;
        }
        case 2:{
            int64_t num = std::bit_cast<int64_t>(divNumer);
            int64_t denom = std::bit_cast<int64_t>(divDenom);
            if(denom == 0){
                divRemResult = std::bit_cast<uint64_t>(num);
                divResult = num >= 0 ? std::bit_cast<uint64_t>((int64_t)-1) : 1;
                break;
            }
            else if(denom == -1 && num < std::numeric_limits<int32_t>::min()){
                divResult = std::bit_cast<uint64_t>(num);
                divRemResult = 0;
                break;
            }
            int64_t result = num / denom;
            int64_t m = num % denom;
            divResult = std::bit_cast<uint64_t>(result);
            divRemResult = std::bit_cast<uint64_t>(m);
            break;
        }
        default: return;
    }
}

void nds::DSMaths::doSqrt()
{
    SQRTCNT &= ~(1u << 15);

    sqrtResult = std::sqrtl((SQRTCNT & 1u) ? sqrtParam : sqrtParam & 0xFFFFFFFF);
}

void nds::DSMaths::clock()
{
    if(remainingDiv > 0){
        remainingDiv--;
        if(remainingDiv == 0){
            doDivision();
        }
    }
    if(remainingSqrt > 0){
        remainingSqrt--;
        if(remainingSqrt == 0){
            doSqrt();
        }
    }
}

Byte nds::DSMaths::onRead(Word addr)
{
    if(addr == 0x4000280){
        return DIVCNT;
    }
    if(addr == 0x4000281){
        return DIVCNT >> 8;
    }
    if(addr >= 0x4000290 && addr < 0x4000298){
        const Word byte = addr - 0x4000290;
        return divNumer >> (8 * byte);
    }
    if(addr >= 0x4000298 && addr < 0x40002A0){
        const Word byte = addr - 0x4000298;
        return divDenom >> (8 * byte);
    }
    if(addr >= 0x40002A0 && addr < 0x40002A8){
        const Word byte = addr - 0x40002A0;
        return divResult >> (8 * byte);
    }
    if(addr >= 0x40002A8 && addr < 0x40002B0){
        const Word byte = addr - 0x40002A8;
        return divRemResult >> (8 * byte);
    }
    if(addr == 0x40002B0){
        return SQRTCNT;
    }
    if(addr == 0x40002B1){
        return SQRTCNT >> 8;
    }
    if(addr >= 0x40002B4 && addr < 0x40002B8){
        const Word byte = addr - 0x40002B4;
        return sqrtResult >> (8 * byte);
    }
    if(addr >= 0x40002B8 && addr < 0x40002C0){
        const Word byte = addr - 0x40002B8;
        return sqrtParam >> (8 * byte);
    }
    return 0;
}

void nds::DSMaths::onWrite(Word addr, Byte val)
{
    if(addr == 0x4000280){
        DIVCNT = (DIVCNT & 0xFF00) | (val & 3u);
        startDivision();
    }
    else if(addr == 0x4000281){
        DIVCNT = (DIVCNT & 0x00FF) | (HalfWord(val & 192u) << 8);
        startDivision();
    }
    else if(addr >= 0x4000290 && addr < 0x4000298){
        const Word byte = addr - 0x4000290;
        divNumer = (divNumer & ~(uint64_t(0xFF) << (8 * byte))) | (uint64_t(val) << (8 * byte));
        startDivision();
    }
    else if(addr >= 0x4000298 && addr < 0x40002A0){
        const Word byte = addr - 0x4000298;
        divDenom = (divDenom & ~(uint64_t(0xFF) << (8 * byte))) | (uint64_t(val) << (8 * byte));
        startDivision();
    }
    else if(addr == 0x40002B0){
        SQRTCNT = (SQRTCNT & 0xFF00) | (val & 1u);
        startSqrt();
    }
    else if(addr == 0x40002B1){
        SQRTCNT = (SQRTCNT & 0x00FF) | (HalfWord(val & 128u) << 8);
        startSqrt();
    }
    else if(addr >= 0x40002B8 && addr < 0x40002C0){
        const Word byte = addr - 0x40002B8;
        sqrtParam = (sqrtParam & ~(uint64_t(0xFF) << (8 * byte))) | (uint64_t(val) << (8 * byte));
        startSqrt();
    }
}
