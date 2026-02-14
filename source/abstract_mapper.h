#pragma once
#include <cstdint>
#include <memory>
#include <string>

enum Mirror{
    MIRROR_VERTICAL,
    MIRROR_HORIZONTAL,
    SINGLE_SCREEN_LOWER,
    SINGLE_SCREEN_UPPER,
    QUAD_SCREEN
};

class Mapper;

// Dieser "abstrakte" Mapper verwaltet nicht den oberen Speicherbereich >= 0x6000 im CPU-Bus (also der Cartridge und PRG-Ram)
// Eine konkrete Mapper-Implementierung ist daher vonnöten
// Das Verhalten was er bereitstellt, wird von allen konkreten Mappern als Fallback genutzt, bzw. dann, wenn deren Verhalten in einem Adressbereich dem Standard entspricht
class AbstractMapper{
    private:
    uint8_t* translatePPUBus(uint8_t* addr);
    // Paletten können nicht vom konkreten Mapper überschrieben werden
    uint8_t** palletteMap;
    protected:
    uint8_t* prgRam;
    unsigned int prgRamSize = 0;
    bool chrRam = false;

    // Rad Racer 2
    uint8_t* hardwiredVram;

    // Weil mapper->cart zum Destruktorzeitpunkt nicht mehr existiert
    std::string name = "";
    bool containsBatteryBackedPRGRAM = false;

    // Prg Ram MUSS bereits erstellt sein, wenn die aufgerufen werden
    void loadSave();
    void saveFile();


    public:
    Mapper* mapper;
    Mirror mirror = MIRROR_VERTICAL;

    // Diese beiden Funktionen werden nur im Adressraum >= 0x6000 aufgerufen, Mappings darunter sind fest
    // Operationen auf CPU-Bus
    virtual void writeRam(uint8_t* addr, uint8_t value) = 0;
    virtual uint8_t readRam(uint8_t* addr) = 0;

    // Operationen auf PPU-Bus
    virtual void writePPU(uint8_t* addr, uint8_t value);
    virtual uint8_t readPPU(uint8_t* addr);

    //MMC3 IRQ (Mapper4)
    virtual void onPPUA12RisingEdge(){};

    AbstractMapper(std::shared_ptr<Mapper> m);
    virtual ~AbstractMapper();
    virtual void reset();
};