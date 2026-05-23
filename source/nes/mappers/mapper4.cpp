#include "mapper4.h"

void Mapper4::bankDataWriteSelect(uint8_t value)
{
    bdwSelect = value;
}

uint8_t *Mapper4::translatePPUBus(uint8_t *addr)
{
    // 2KiB Bänke ignorieren unterstes Bit
    registers[0] = registers[0] & 0b11111110;
    registers[1] = registers[1] & 0b11111110;

    if(chrA12Inversion){ // Zwei 2KiB Bänke bei 0x1000-0x1FFF, Vier 1KiB Bänke bei 0x000-0x0FFF
        if((uintptr_t)addr < 0x0400){ //R2 Index 1KiB
            uint16_t offset = (uintptr_t)addr - 0x0;
            return mapper.lock()->cart->chrRom.data() +                         offset + (registers[2] * 0x400);
        }
        else if((uintptr_t)addr < 0x0800){ //R3 Index 1KiB
            uint16_t offset = (uintptr_t)addr - 0x400;
            return mapper.lock()->cart->chrRom.data() +                         offset + (registers[3] * 0x400);
        }
        else if((uintptr_t)addr < 0x0C00){ //R4 Index 1KiB
            uint16_t offset = (uintptr_t)addr - 0x800;
            return mapper.lock()->cart->chrRom.data() +                         offset + (registers[4] * 0x400);
        }
        else if((uintptr_t)addr < 0x1000){ //R5 Index 1KiB
            uint16_t offset = (uintptr_t)addr - 0xC00;
            return mapper.lock()->cart->chrRom.data() +                         offset + (registers[5] * 0x400);
        }
        if((uintptr_t)addr < 0x1800){ //R0 Index 2KiB
            uint16_t offset = (uintptr_t)addr - 0x1000;
            return mapper.lock()->cart->chrRom.data() +                         offset + (registers[0] * 0x400);
        }
        else{ //R1 Index 2KiB
            uint16_t offset = (uintptr_t)addr - 0x1800;
            return mapper.lock()->cart->chrRom.data() +                         offset + (registers[1] * 0x400);
        }
    }
    else{ // Zwei 2KiB Bänke bei 0x000-0x0FFF, Vier 1KiB Bänke bei 0x1000-0x1FFF
        if((uintptr_t)addr < 0x0800){ //R0 Index 2KiB
            uint16_t offset = (uintptr_t)addr - 0x0;
            return mapper.lock()->cart->chrRom.data() +                         offset + (registers[0] * 0x400);
        }
        else if((uintptr_t)addr < 0x1000){ //R1 Index 2KiB
            uint16_t offset = (uintptr_t)addr - 0x800;
            return mapper.lock()->cart->chrRom.data() +                         offset + (registers[1] * 0x400);
        }
        else if((uintptr_t)addr < 0x1400){ //R2 Index 1KiB
            uint16_t offset = (uintptr_t)addr - 0x1000;
            return mapper.lock()->cart->chrRom.data() +                         offset + (registers[2] * 0x400);
        }
        else if((uintptr_t)addr < 0x1800){ //R3 Index 1KiB
            uint16_t offset = (uintptr_t)addr - 0x1400;
            return mapper.lock()->cart->chrRom.data() +                         offset + (registers[3] * 0x400);
        }
        else if((uintptr_t)addr < 0x1C00){ //R4 Index 1KiB
            uint16_t offset = (uintptr_t)addr - 0x1800;
            return mapper.lock()->cart->chrRom.data() +                         offset + (registers[4] * 0x400);
        }
        else{ //R5 Index 1KiB
            uint16_t offset = (uintptr_t)addr - 0x1C00;
            return mapper.lock()->cart->chrRom.data() +                         offset + (registers[5] * 0x400);
        }
    }
}

void Mapper4::writeRam(uint8_t *addr, uint8_t value)
{
    if((uintptr_t)addr < 0x8000){
        // if(prgRamEnable)
            prgRam[(uintptr_t)addr - 0x6000] = value;
    }
    else if ((uintptr_t)addr < 0xA000){ // Konfiguration 1
        if((uintptr_t)addr % 2 == 0){ // gerade, Bank select
            bankDataWriteSelect(value & 0b00000111);
            // prgRamEnable = value & 0b00100000;
            prgRomBankMode = value & 0b01000000;
            chrA12Inversion = value & 0b10000000;
        }
        else{ // ungerade
            registers[bdwSelect] = value;
        }
    }
    else if((uintptr_t)addr < 0xC000){
        if((uintptr_t)addr % 2 == 0){ // gerade, Nametable-Ausrichtung
            if(!mapper.lock()->cart->header.flags6.hasAlternativeNametableLayout())
                mirror = value & 1 ? MIRROR_HORIZONTAL : MIRROR_VERTICAL;
            else mirror = QUAD_SCREEN;
        }
        else{ // ungerade
            // Prg Ram Protect
            // Nicht implementiert für Kompatibilität mit MMC6
        }
    }
    else if((uintptr_t)addr < 0xE000){
        if((uintptr_t)addr % 2 == 0){ // gerade, IRQ Latch
            IRQCounterReload = value;
        }
        else{ // ungerade, IRQ Reload
            IRQCounter = 0;
        }
    }
    else{
        if((uintptr_t)addr % 2 == 0){ // gerade, IRQ disable
            disableIRQ = true;
        }
        else{ // ungerade, IRQ enable
            disableIRQ = false;
        }
    }
}

uint8_t Mapper4::readRam(uint8_t *addr)
{

    // Obere beide Bits ignorieren
    registers[6] = registers[6] & 0b00111111;
    registers[7] = registers[7] & 0b00111111;

    if((uintptr_t)addr < 0x8000){
        // if(prgRamEnable)
            return prgRam[(uintptr_t)addr - 0x6000];
        // else return 0;
    }
    else if((uintptr_t)addr < 0xA000){
        if(!prgRomBankMode){ // tauschbar durch registers[6]
            return mapper.lock()->cart->prgRom[(uintptr_t)addr - 0x8000 + ((registers[6]) * 0x2000)];
        }
        else{ // fest, vorletzte
            return mapper.lock()->cart->prgRom[(uintptr_t)addr - 0x8000 + ((mapper.lock()->cart->header.PRGROMSize - 1) * 0x4000)];
        }
    }
    else if((uintptr_t)addr < 0xC000){ // tauschbar durch registers[7]
        return mapper.lock()->cart->prgRom[(uintptr_t)addr - 0xA000 + ((registers[7]) * 0x2000)];
    }
    else if((uintptr_t)addr < 0xE000){ // fest, vorletzte Bank oder tauschbar
        if(!prgRomBankMode){ // fest, vorletzte
            return mapper.lock()->cart->prgRom[(uintptr_t)addr - 0xC000 + ((mapper.lock()->cart->header.PRGROMSize - 1) * 0x4000)];
        }
        else{ // tauschbar durch registers[6]
            return mapper.lock()->cart->prgRom[(uintptr_t)addr - 0xC000 + ((registers[6]) * 0x2000)];
        }
    }
    else{ // fest, letzte Bank
        return mapper.lock()->cart->prgRom[(uintptr_t)addr - 0xE000 + ((mapper.lock()->cart->header.PRGROMSize - 1) * 0x4000) + 0x2000];
    }
}

void Mapper4::writePPU(uint8_t *addr, uint8_t value)
{
    if((uintptr_t)addr < 0x2000){
        *translatePPUBus(addr) = value;
    }
    else AbstractMapper::writePPU(addr, value);
}

uint8_t Mapper4::readPPU(uint8_t *addr)
{
    if((uintptr_t)addr < 0x2000){
        return *translatePPUBus(addr);
    }

    return AbstractMapper::readPPU(addr);
}

void Mapper4::onPPUA12RisingEdge()
{

    // Filter A12
    // MUSS NOCH IMPLEMENTIERT WERDEN

    if(IRQCounter==0){
        IRQCounter = IRQCounterReload;
    }
    else{
        IRQCounter--;
    };

    if(IRQCounter==0 && !disableIRQ){
        mapper.lock()->pullIRQ();
    }
}

Mapper4::Mapper4(std::shared_ptr<Mapper> m) : AbstractMapper(m)
{
    prgRamSize = 0x2000;
    prgRam = std::vector<uint8_t>(prgRamSize, 0);
    this->reset();

    AbstractMapper::loadSave();
}

Mapper4::~Mapper4()
{
    AbstractMapper::saveFile();
}

void Mapper4::reset()
{
    bdwSelect = 0;

    prgRomBankMode = false;
    chrA12Inversion = false;
    IRQCounterReload = 0;
    IRQCounter = 0;
    disableIRQ = false;

    for(int i = 0; i < 8; i++){
        registers[i] = 0;
    }
}
