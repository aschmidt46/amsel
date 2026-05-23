#pragma once
#include <cstdint>
#include "nes_file.h"
#include "6502.h"
#include "ppu.h"
#include <limits>
#include "controller.h"
#include "apu/apu.h"
#include "mappers/abstract_mapper.h"
#include "mappers/mappers.h"

constexpr const size_t ADDRSPACE = 1 + (size_t) std::numeric_limits<uint16_t>::max();

class Cpu;
class Ppu;
class Apu;
class Controller;
class AbstractMapper;
class Mapper : virtual public std::enable_shared_from_this<Mapper>{
    private:
    std::unique_ptr<AbstractMapper> mapperImplementation = nullptr;
    public:

    
    std::weak_ptr<Cpu> cpu;
    std::weak_ptr<Ppu> ppu;
    std::weak_ptr<Apu> apu;
    std::weak_ptr<Controller> controller1;
    std::weak_ptr<Controller> controller2;

    std::shared_ptr<NESFile> cart = nullptr;

    
    
    uint8_t controller[2];
    uint8_t controller_state[2];
    uint8_t io[0x18];



    bool chrRAM = false;

    Mapper(std::weak_ptr<Cpu> cpu, std::weak_ptr<Ppu> ppu, std::weak_ptr<Apu> apu);
    ~Mapper(){};

    bool changeCart(std::shared_ptr<NESFile> cartridge);
    void connectController(std::weak_ptr<Controller> controller1, std::weak_ptr<Controller> controller2);

    uint8_t* getMemoryMapping(uint8_t* addr);

    uint8_t read(uint8_t* address);
    void write(uint8_t* address, uint8_t value);
    uint8_t readVRAM(uint8_t* address);
    void writeVRAM(uint8_t* address, uint8_t value);
    void pullNMI();
    void pullIRQ();
    void riseA12();
    void reset();

    bool canSave();
    std::vector<uint8_t> getSaveData();
};
