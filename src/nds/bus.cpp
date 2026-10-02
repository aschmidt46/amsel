#include "bus.h"

using namespace nds;

void SharedBus::init()
{
    bus7 = std::make_shared<Bus7>(this);
    bus9 = std::make_shared<Bus9>(this);
}

void nds::SharedBus::setIF(int bit, bool val)
{
    IF = (IF & ~(1ui32 << bit)) | (val << bit);
}

Word nds::SharedBus::getIE()
{
    return IE;
}

Word nds::SharedBus::getIF()
{
    return IF;
}

bool nds::SharedBus::hasIME()
{
    return IME.raw & 1u;
}

void nds::SharedBus::setHalt()
{
    isHalted = true;
}
