#pragma once
#include <cstdint>
#include <memory>
#include <string>

enum Mirror{
    MIRROR_VERTICAL,
    MIRROR_HORIZONTAL
};

class Mapper;

class AbstractMapper{
    protected:
    uint8_t* prgRam;
    unsigned int prgRamSize = 0;

    // Weil mapper->cart zum Destruktorzeitpunkt nicht mehr existiert
    std::string name = "";
    bool containsBatteryBackedPRGRAM = false;

    // Prg Ram MUSS bereits erstellt sein, wenn die aufgerufen werden
    void loadSave();
    void saveFile();

    public:
    std::shared_ptr<Mapper> mapper;
    Mirror mirror = MIRROR_VERTICAL;

    // Diese beiden Funktionen werden nur im Adressraum >= 0x8000 aufgerufen
    virtual void writeRam(uint8_t* addr, uint8_t value) = 0;
    virtual uint8_t readRam(uint8_t* addr) = 0;

    virtual void writePPU(uint8_t* addr, uint8_t value);
    virtual uint8_t readPPU(uint8_t* addr);

    AbstractMapper(std::shared_ptr<Mapper> m);
    virtual ~AbstractMapper() = default;
};