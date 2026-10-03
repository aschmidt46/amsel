#include "bus.h"

using namespace nds;

void nds::SharedBus::PPUEnteredHBlank()
{
}

void nds::SharedBus::PPUEnteredVBlank()
{
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
    bus9 = std::make_shared<Bus9>(this);
}

void nds::SharedBus::setIF(int bit, bool val)
{
    bus7->setIF(bit, val);   
    bus9->setIF(bit, val);
}

void nds::SharedBus::setHalt()
{
    isHalted = true;
}

std::pair<float, float> nds::SharedBus::getSample()
{
    return {0,0};
}

void nds::SharedBus::clock()
{
}
