#pragma once

#include "abstract_mapper.h"
#include "../mapper.h"
#include <fstream>

// The Lone Ranger untersuchen
// Power Blade 2
// G.I. Joe The Atlantis Factor
// Contra Force

class Mapper4 : public AbstractMapper {
    
    void bankDataWriteSelect(uint8_t value);

    uint8_t* bdwSelect = nullptr;

    // Kompatibilität mit MMC6: Ram immer schreibbar
    // bool prgRamEnable = true;
    bool prgRomBankMode = false;
    bool chrA12Inversion = false;
    uint8_t IRQCounterReload = 0;
    uint8_t IRQCounter = 0;
    bool disableIRQ = false;

    //CHR Bänke
    uint8_t R0 = 0;
    uint8_t R1 = 0;
    uint8_t R2 = 0;
    uint8_t R3 = 0;
    uint8_t R4 = 0;
    uint8_t R5 = 0;

    // PRG Bänke
    uint8_t R6 = 0;
    uint8_t R7 = 0;


    uint8_t* translatePPUBus(uint8_t* addr);


    public:
    void writeRam(uint8_t* addr, uint8_t value) override;
    uint8_t readRam(uint8_t* addr) override;

    void writePPU(uint8_t* addr, uint8_t value) override;
    uint8_t readPPU(uint8_t* addr) override;

    void onPPUA12RisingEdge() override;

    Mapper4(std::shared_ptr<Mapper> m);
    ~Mapper4() override;
    void reset() override;

};