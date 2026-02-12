#pragma once

#include "../abstract_mapper.h"
#include "../mapper.h"

class Mapper1 : AbstractMapper {
    uint8_t shiftReg = 0;

    void shiftInto(bool value);
    bool shiftIsFull();

    void changeBanks(uint16_t bank, uint8_t value);

    // Control
    bool chrRomBankMode = false;
    uint8_t prgRomBankMode = 3;

    // Chr banks
    uint8_t chrBank0 = 0;
    uint8_t chrBank1 = 1; //?

    // Prg Bank
    bool enablePRGRam = false;
    uint8_t prgRomBankSelect = 0;

    uint8_t* translatePPUBus(uint8_t* addr);


    public:
    void writeRam(uint8_t* addr, uint8_t value) override;
    uint8_t readRam(uint8_t* addr) override;

    void writePPU(uint8_t* addr, uint8_t value) override;
    uint8_t readPPU(uint8_t* addr) override;

    Mapper1(std::shared_ptr<Mapper> m);
    ~Mapper1() override;
    void reset() override;

};