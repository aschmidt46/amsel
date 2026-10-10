#include "lcd.h"

using namespace nds;

static constexpr uint8_t BANK_A_BEGIN = 0;  // 128K
static constexpr uint8_t BANK_B_BEGIN = 8;  // 128K
static constexpr uint8_t BANK_C_BEGIN = 16; // 128K
static constexpr uint8_t BANK_D_BEGIN = 24; // 128K
static constexpr uint8_t BANK_E_BEGIN = 32; // 64K
static constexpr uint8_t BANK_F_BEGIN = 36; // 16K
static constexpr uint8_t BANK_G_BEGIN = 37; // 16K
static constexpr uint8_t BANK_H_BEGIN = 38; // 32K
static constexpr uint8_t BANK_I_BEGIN = 40; // 16K

static constexpr uint8_t BANK_A_SIZE = 8;
static constexpr uint8_t BANK_B_SIZE = 8;
static constexpr uint8_t BANK_C_SIZE = 8;
static constexpr uint8_t BANK_D_SIZE = 8;
static constexpr uint8_t BANK_E_SIZE = 4;
static constexpr uint8_t BANK_F_SIZE = 1;
static constexpr uint8_t BANK_G_SIZE = 1;
static constexpr uint8_t BANK_H_SIZE = 2;
static constexpr uint8_t BANK_I_SIZE = 1;

static constexpr uint8_t BANK_A_END = BANK_A_BEGIN + BANK_A_SIZE;
static constexpr uint8_t BANK_B_END = BANK_B_BEGIN + BANK_B_SIZE;
static constexpr uint8_t BANK_C_END = BANK_C_BEGIN + BANK_C_SIZE;
static constexpr uint8_t BANK_D_END = BANK_D_BEGIN + BANK_D_SIZE;
static constexpr uint8_t BANK_E_END = BANK_E_BEGIN + BANK_E_SIZE;
static constexpr uint8_t BANK_F_END = BANK_F_BEGIN + BANK_F_SIZE;
static constexpr uint8_t BANK_G_END = BANK_G_BEGIN + BANK_G_SIZE;
static constexpr uint8_t BANK_H_END = BANK_H_BEGIN + BANK_H_SIZE;
static constexpr uint8_t BANK_I_END = BANK_I_BEGIN + BANK_I_SIZE;

Byte nds::LCD::readVramBankControl(Word addr){
    addr -= 0x4000240;
    if(addr < 10 && addr != 7){
        return vramControl[addr].raw;
    }
    return 0;
}

void nds::LCD::writeVramBankControl(Word addr, Byte val)
{
    addr -= 0x4000240;
    if(addr < 10){
        // Alte Bank aus PageTable entfernen
        switch(addr){
            case 0:
                clearVramBank<0>();
                break;
            case 1:
                clearVramBank<1>();
                break;
            case 2:
                clearVramBank<2>();
                break;
            case 3:
                clearVramBank<3>();
                break;
            case 4:
                clearVramBank<4>();
                break;
            case 5:
                clearVramBank<5>();
                break;
            case 6:
                clearVramBank<6>();
                break;
            case 8:
                clearVramBank<8>();
                break;
            case 9:
                clearVramBank<9>();
                break;
            default: break;
        }
        vramControl[addr].raw = val;
        if(vramControl[addr].state.enable){
            // Neue Bank hinzufügen
            switch(addr){
                case 0:
                    switchVramBank<0>();
                    break;
                case 1:
                    switchVramBank<1>();
                    break;
                case 2:
                    switchVramBank<2>();
                    break;
                case 3:
                    switchVramBank<3>();
                    break;
                case 4:
                    switchVramBank<4>();
                    break;
                case 5:
                    switchVramBank<5>();
                    break;
                case 6:
                    switchVramBank<6>();
                    break;
                case 8:
                    switchVramBank<8>();
                    break;
                case 9:
                    switchVramBank<9>();
                    break;
                default: break;
            }
        }
    }
}

Byte nds::LCD::getVRAMSTAT()
{
    Byte c = Byte(vramControl[2].state.enable && vramControl[2].state.mst == 2);
    Byte d = Byte(vramControl[3].state.enable && vramControl[3].state.mst == 2);
    return c | (d << 1);
}

// ------------------------------------------------------------------------------------------------------------------- A
template<>
void LCD::switchVramBank<0>(){ // 8 "Bänke" (16KB Blöcke) in Bank A
    const int BankIndex = 0; // A == 0
    auto &control = vramControl[BankIndex].state;
    control.mst &= 0b11u; // "Bit 2 not used by vram a,b,h,i"
    if(control.mst == 0){
        for(Byte i = 0; i < BANK_A_SIZE; i++){
            const Word index = 512 + i;
            pageTable[index].state.bankID = BANK_A_BEGIN + i; // 0x06800000... - = 512 in Pagetable
            pageTable[index].state.valid = true;
        }
    }
    else if(control.mst == 1){
        for(Byte i = 0; i < BANK_A_SIZE; i++){
            const Word index = i + 8 * control.ofs;
            pageTable[index].state.bankID = BANK_A_BEGIN + i; // offset 0x20000 = 8 in Pagetable
            pageTable[index].state.valid = true;
        }
    }
    else if(control.mst == 2){
        const Byte ofs0 = control.ofs & 0b1u;
        for(Byte i = 0; i < BANK_A_SIZE; i++){
            const Word index = 256 + i + 8 * ofs0;
            pageTable[index].state.bankID = BANK_A_BEGIN + i;
            pageTable[index].state.valid = true;
        }
    }
    else if(control.mst == 3){
        engine3D.textureRearPlane[control.ofs] = {.ptr = vram.data(), .allocatedTo = BankIndex}; // allocated = A(0)
    }
}

template<>
void LCD::clearVramBank<0>(){
    const int BankIndex = 0; // A == 0
    auto &control = vramControl[BankIndex].state;
    control.mst &= 0b11u;
    if(control.mst == 0){
        for(Byte i = 0; i < BANK_A_SIZE; i++){
            const Word index = 512 + i;
            // NUR entfernen, wenn das Mapping zu Bank A gehört und nicht von anderer Bank überschrieben wurde
            if(pageTable[index].state.bankID >= BANK_A_BEGIN && pageTable[index].state.bankID < BANK_A_END){
                pageTable[index].state.valid = false;
            }
        }
    }
    else if(control.mst == 1){
        for(Byte i = 0; i < BANK_A_SIZE; i++){
            const Word index = i + 8 * control.ofs;
            if(pageTable[index].state.bankID >= BANK_A_BEGIN && pageTable[index].state.bankID < BANK_A_END){
                pageTable[index].state.valid = false;
            }
        }
    }
    else if(control.mst == 2){
        const Byte ofs0 = control.ofs & 0b1u;
        for(Byte i = 0; i < BANK_A_SIZE; i++){
            const Word index = 256 + i + 8 * ofs0;
            if(pageTable[index].state.bankID >= BANK_A_BEGIN && pageTable[index].state.bankID < BANK_A_END){
                pageTable[index].state.valid = false;
            }
        }
    }
    else if(control.mst == 3){
        if(engine3D.textureRearPlane[control.ofs].allocatedTo == BankIndex) // Nur, falls Allokation an Bank A (0)
            engine3D.textureRearPlane[control.ofs].ptr = nullptr;
    }
}

// ------------------------------------------------------------------------------------------------------------------- B
template<>
void LCD::switchVramBank<1>(){
    const int BankIndex = 1; // B == 1
    auto &control = vramControl[BankIndex].state;
    control.mst &= 0b11u; // "Bit 2 not used by vram a,b,h,i"
    if(control.mst == 0){
        for(Byte i = 0; i < BANK_B_SIZE; i++){
            const Word index = 512 + BANK_B_BEGIN + i;
            pageTable[index].state.bankID = BANK_B_BEGIN + i; // 0x06820000... - = 512 + 8 in Pagetable
            pageTable[index].state.valid = true;
        }
    }
    else if(control.mst == 1){
        for(Byte i = 0; i < BANK_B_SIZE; i++){
            const Word index = i + 8 * control.ofs;
            pageTable[index].state.bankID = BANK_B_BEGIN + i;
            pageTable[index].state.valid = true;
        }
    }
    else if(control.mst == 2){
        const Byte ofs0 = control.ofs & 0b1u;
        for(Byte i = 0; i < BANK_B_SIZE; i++){
            const Word index = 256 + i + 8 * ofs0;
            pageTable[index].state.bankID = BANK_B_BEGIN + i;
            pageTable[index].state.valid = true;
        }
    }
    else if(control.mst == 3){
        engine3D.textureRearPlane[control.ofs] = {.ptr = vram.data() + BANK_B_BEGIN * 0x4000, .allocatedTo = BankIndex}; // allocated = B(1)
    }
}

template<>
void LCD::clearVramBank<1>(){
    const int BankIndex = 1; // B == 1
    auto &control = vramControl[BankIndex].state;
    control.mst &= 0b11u;
    if(control.mst == 0){
        for(Byte i = 0; i < BANK_B_SIZE; i++){
            const Word index = 512 + BANK_B_BEGIN + i;
            if(pageTable[index].state.bankID >= BANK_B_BEGIN && pageTable[index].state.bankID < BANK_B_END){
                pageTable[index].state.valid = false;
            }
        }
    }
    else if(control.mst == 1){
        for(Byte i = 0; i < BANK_B_SIZE; i++){
            const Word index = i + 8 * control.ofs;
            if(pageTable[index].state.bankID >= BANK_B_BEGIN && pageTable[index].state.bankID < BANK_B_END){
                pageTable[index].state.valid = false;
            }
        }
    }
    else if(control.mst == 2){
        const Byte ofs0 = control.ofs & 0b1u;
        for(Byte i = 0; i < BANK_B_SIZE; i++){
            const Word index = 256 + i + 8 * ofs0;
            if(pageTable[index].state.bankID >= BANK_B_BEGIN && pageTable[index].state.bankID < BANK_B_END){
                pageTable[index].state.valid = false;
            }
        }
    }
    else if(control.mst == 3){
        if(engine3D.textureRearPlane[control.ofs].allocatedTo == BankIndex) // Nur, falls Allokation an Bank B (1)
            engine3D.textureRearPlane[control.ofs].ptr = nullptr;
    }
}

// ------------------------------------------------------------------------------------------------------------------- C
template<>
void LCD::switchVramBank<2>(){
    const int BankIndex = 2; // C == 2
    auto &control = vramControl[BankIndex].state;
    if(control.mst == 0){
        for(Byte i = 0; i < BANK_C_SIZE; i++){
            const Word index = 512 + BANK_C_BEGIN + i;
            pageTable[index].state.bankID = BANK_C_BEGIN + i;
            pageTable[index].state.valid = true;
        }
    }
    else if(control.mst == 1){
        for(Byte i = 0; i < BANK_C_SIZE; i++){
            const Word index = i + 8 * control.ofs;
            pageTable[index].state.bankID = BANK_C_BEGIN + i;
            pageTable[index].state.valid = true;
        }
    }
    // Arm7 MAP!!
    else if(control.mst == 2){
        const Byte ofs0 = control.ofs & 0b1u;
        for(Byte i = 0; i < BANK_C_SIZE; i++){
            const Word index = i + 8 * ofs0;
            arm7PageTable[index].state.bankID = BANK_C_BEGIN + i;
            arm7PageTable[index].state.valid = true;
        }
    }
    else if(control.mst == 3){
        engine3D.textureRearPlane[control.ofs] = {.ptr = vram.data() + BANK_C_BEGIN * 0x4000, .allocatedTo = BankIndex};
    }
    else if(control.mst == 4){
        for(Byte i = 0; i < BANK_C_SIZE; i++){
            const Word index = 128 + i; // 128 == 0x062...?
            pageTable[index].state.bankID = BANK_C_BEGIN + i;
            pageTable[index].state.valid = true;
        }
    }
}

template<>
void LCD::clearVramBank<2>(){
    const int BankIndex = 2; // C == 2
    auto &control = vramControl[BankIndex].state;
    if(control.mst == 0){
        for(Byte i = 0; i < BANK_C_SIZE; i++){
            const Word index = 512 + BANK_C_BEGIN + i;
            if(pageTable[index].state.bankID >= BANK_C_BEGIN && pageTable[index].state.bankID < BANK_C_END){
                pageTable[index].state.valid = false;
            }
        }
    }
    else if(control.mst == 1){
        for(Byte i = 0; i < BANK_C_SIZE; i++){
            const Word index = i + 8 * control.ofs;
            if(pageTable[index].state.bankID >= BANK_C_BEGIN && pageTable[index].state.bankID < BANK_C_END){
                pageTable[index].state.valid = false;
            }
        }
    }
    else if(control.mst == 2){
        const Byte ofs0 = control.ofs & 0b1u;
        for(Byte i = 0; i < BANK_C_SIZE; i++){
            const Word index = i + 8 * ofs0;
            if(arm7PageTable[index].state.bankID >= BANK_C_BEGIN && arm7PageTable[index].state.bankID < BANK_C_END){
                arm7PageTable[index].state.valid = false;
            }
        }
    }
    else if(control.mst == 3){
        if(engine3D.textureRearPlane[control.ofs].allocatedTo == BankIndex)
            engine3D.textureRearPlane[control.ofs].ptr = nullptr;
    }
    else if(control.mst == 4){
        for(Byte i = 0; i < BANK_C_SIZE; i++){
            const Word index = 128 + i;
            if(pageTable[index].state.bankID >= BANK_C_BEGIN && pageTable[index].state.bankID < BANK_C_END){
                pageTable[index].state.valid = false;
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------------------------- D
template<>
void LCD::switchVramBank<3>(){
    const int BankIndex = 3; // D == 3
    auto &control = vramControl[BankIndex].state;
    if(control.mst == 0){
        for(Byte i = 0; i < BANK_D_SIZE; i++){
            const Word index = 512 + BANK_D_BEGIN + i;
            pageTable[index].state.bankID = BANK_D_BEGIN + i;
            pageTable[index].state.valid = true;
        }
    }
    else if(control.mst == 1){
        for(Byte i = 0; i < BANK_D_SIZE; i++){
            const Word index = i + 8 * control.ofs;
            pageTable[index].state.bankID = BANK_D_BEGIN + i;
            pageTable[index].state.valid = true;
        }
    }
    // Arm7 MAP!!
    else if(control.mst == 2){
        const Byte ofs0 = control.ofs & 0b1u;
        for(Byte i = 0; i < BANK_D_SIZE; i++){
            const Word index = i + 8 * ofs0;
            arm7PageTable[index].state.bankID = BANK_D_BEGIN + i;
            arm7PageTable[index].state.valid = true;
        }
    }
    else if(control.mst == 3){
        engine3D.textureRearPlane[control.ofs] = {.ptr = vram.data() + BANK_D_BEGIN * 0x4000, .allocatedTo = BankIndex};
    }
    else if(control.mst == 4){
        for(Byte i = 0; i < BANK_D_SIZE; i++){
            const Word index = 384 + i; // 284 == 0x066...?
            pageTable[index].state.bankID = BANK_D_BEGIN + i;
            pageTable[index].state.valid = true;
        }
    }
}

template<>
void LCD::clearVramBank<3>(){
    const int BankIndex = 3; // D == 3
    auto &control = vramControl[BankIndex].state;
    if(control.mst == 0){
        for(Byte i = 0; i < BANK_D_SIZE; i++){
            const Word index = 512 + BANK_D_BEGIN + i;
            if(pageTable[index].state.bankID >= BANK_D_BEGIN && pageTable[index].state.bankID < BANK_D_END){
                pageTable[index].state.valid = false;
            }
        }
    }
    else if(control.mst == 1){
        for(Byte i = 0; i < BANK_D_SIZE; i++){
            const Word index = i + 8 * control.ofs;
            if(pageTable[index].state.bankID >= BANK_D_BEGIN && pageTable[index].state.bankID < BANK_D_END){
                pageTable[index].state.valid = false;
            }
        }
    }
    else if(control.mst == 2){
        const Byte ofs0 = control.ofs & 0b1u;
        for(Byte i = 0; i < BANK_D_SIZE; i++){
            const Word index = i + 8 * ofs0;
            if(arm7PageTable[index].state.bankID >= BANK_D_BEGIN && arm7PageTable[index].state.bankID < BANK_D_END){
                arm7PageTable[index].state.valid = false;
            }
        }
    }
    else if(control.mst == 3){
        if(engine3D.textureRearPlane[control.ofs].allocatedTo == BankIndex)
            engine3D.textureRearPlane[control.ofs].ptr = nullptr;
    }
    else if(control.mst == 4){
        for(Byte i = 0; i < BANK_D_SIZE; i++){
            const Word index = 384 + i;
            if(pageTable[index].state.bankID >= BANK_D_BEGIN && pageTable[index].state.bankID < BANK_D_END){
                pageTable[index].state.valid = false;
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------------------------- E
template<>
void LCD::switchVramBank<4>(){
    const int BankIndex = 4; // E == 4
    auto &control = vramControl[BankIndex].state;
    if(control.mst == 0){
        for(Byte i = 0; i < BANK_E_SIZE; i++){
            const Word index = 512 + BANK_E_BEGIN + i;
            pageTable[index].state.bankID = BANK_E_BEGIN + i;
            pageTable[index].state.valid = true;
        }
    }
    else if(control.mst == 1){
        for(Byte i = 0; i < BANK_E_SIZE; i++){
            const Word index = i;
            pageTable[index].state.bankID = BANK_E_BEGIN + i;
            pageTable[index].state.valid = true;
        }
    }
    else if(control.mst == 2){
        for(Byte i = 0; i < BANK_E_SIZE; i++){
            const Word index = 256 + i;
            pageTable[index].state.bankID = BANK_E_BEGIN + i;
            pageTable[index].state.valid = true;
        }
    }
    else if(control.mst == 3){
        for(Byte i = 0; i < BANK_E_SIZE; i++){
            engine3D.texturePalette[i] = {.ptr = vram.data() + BANK_E_BEGIN * 0x4000 + i * 0x4000, .allocatedTo = BankIndex};
        }
    }
    else if(control.mst == 4){
        for(Byte i = 0; i < 4; i++){
            engineA.bgExtendedPalette[i] = {.ptr = vram.data() + BANK_E_BEGIN * 0x4000 + i * 0x2000, .allocatedTo = BankIndex}; // 8KB Bänke, Index Offset = 8KB
        }
    }
}

template<>
void LCD::clearVramBank<4>(){
    const int BankIndex = 4; // E == 4
    auto &control = vramControl[BankIndex].state;
    if(control.mst == 0){
        for(Byte i = 0; i < BANK_E_SIZE; i++){
            const Word index = 512 + BANK_E_BEGIN + i;
            if(pageTable[index].state.bankID >= BANK_E_BEGIN && pageTable[index].state.bankID < BANK_E_END){
                pageTable[index].state.valid = false;
            }
        }
    }
    else if(control.mst == 1){
        for(Byte i = 0; i < BANK_E_SIZE; i++){
            const Word index = i;
            if(pageTable[index].state.bankID >= BANK_E_BEGIN && pageTable[index].state.bankID < BANK_E_END){
                pageTable[index].state.valid = false;
            }
        }
    }
    else if(control.mst == 2){
        for(Byte i = 0; i < BANK_E_SIZE; i++){
            const Word index = 256 + i;
            if(pageTable[index].state.bankID >= BANK_E_BEGIN && pageTable[index].state.bankID < BANK_E_END){
                pageTable[index].state.valid = false;
            }
        }
    }
    else if(control.mst == 3){
        for(Byte i = 0; i < BANK_E_SIZE; i++){
            if(engine3D.texturePalette[i].allocatedTo == BankIndex)
                engine3D.texturePalette[i].ptr = nullptr;
        }
    }
    else if(control.mst == 4){
        for(Byte i = 0; i < 4; i++){
            if(engineA.bgExtendedPalette[i].allocatedTo == BankIndex)
                engineA.bgExtendedPalette[i].ptr = nullptr;
        }
    }
}

// ------------------------------------------------------------------------------------------------------------------- F
template<>
void LCD::switchVramBank<5>(){
    const int BankIndex = 5; // F == 5
    auto &control = vramControl[BankIndex].state;

    const Byte ofs0 = control.ofs & 1u;
    const Byte ofs1 = (control.ofs >> 1) & 1u;

    if(control.mst == 0){
        const Word index = 512 + BANK_F_BEGIN;
        pageTable[index].state.bankID = BANK_F_BEGIN;
        pageTable[index].state.valid = true;
    }
    else if(control.mst == 1){
        const Word index = ofs0 + 4 * ofs1;
        pageTable[index].state.bankID = BANK_F_BEGIN;
        pageTable[index].state.valid = true;
    }
    else if(control.mst == 2){
        const Word index = 256 + ofs0 + 4 * ofs1;
        pageTable[index].state.bankID = BANK_F_BEGIN;
        pageTable[index].state.valid = true;
    }
    else if(control.mst == 3){
        const Word index = ofs0 + 4 * ofs1;
        engine3D.texturePalette[index] = {.ptr = vram.data() + BANK_F_BEGIN * 0x4000, .allocatedTo = BankIndex};
    }
    else if(control.mst == 4){
        const Word ofs = std::min(control.ofs, (Byte)1u);
        const Word index = ofs * 2; // 0 = 0..1, 1 = 2..3
        engineA.bgExtendedPalette[index] = {.ptr = vram.data() + BANK_F_BEGIN * 0x4000, .allocatedTo = BankIndex};
        engineA.bgExtendedPalette[index + 1] = {.ptr = vram.data() + BANK_F_BEGIN * 0x4000 + 0x2000, .allocatedTo = BankIndex};
    }
    else if(control.mst == 5){
        engineA.objExtendedPalette = {.ptr = vram.data() + BANK_F_BEGIN * 0x4000, .allocatedTo = BankIndex};
    }
}

template<>
void LCD::clearVramBank<5>(){
    const int BankIndex = 5; // F == 5
    auto &control = vramControl[BankIndex].state;

    const Byte ofs0 = control.ofs & 1u;
    const Byte ofs1 = (control.ofs >> 1) & 1u;

    if(control.mst == 0){
        const Word index = 512 + BANK_F_BEGIN;
        if(pageTable[index].state.bankID >= BANK_F_BEGIN && pageTable[index].state.bankID < BANK_F_END){
            pageTable[index].state.valid = false;
        }
    }
    else if(control.mst == 1){
        const Word index = ofs0 + 4 * ofs1;
        if(pageTable[index].state.bankID >= BANK_F_BEGIN && pageTable[index].state.bankID < BANK_F_END){
            pageTable[index].state.valid = false;
        }
    }
    else if(control.mst == 2){
        const Word index = 256 + ofs0 + 4 * ofs1;
        if(pageTable[index].state.bankID >= BANK_F_BEGIN && pageTable[index].state.bankID < BANK_F_END){
            pageTable[index].state.valid = false;
        }
    }
    else if(control.mst == 3){
        const Word index = ofs0 + 4 * ofs1;
        if(engine3D.texturePalette[index].allocatedTo == BankIndex)
            engine3D.texturePalette[index].ptr = nullptr;
    }
    else if(control.mst == 4){
        const Word ofs = std::min(control.ofs, (Byte)1u);
        const Word index = ofs * 2; // 0 = 0..1, 1 = 2..3
        if(engineA.bgExtendedPalette[index].allocatedTo == BankIndex)
            engineA.bgExtendedPalette[index].ptr = nullptr;
        if(engineA.bgExtendedPalette[index + 1].allocatedTo == BankIndex)
            engineA.bgExtendedPalette[index + 1].ptr = nullptr;
    }
    else if(control.mst == 5){
        if(engineA.objExtendedPalette.allocatedTo == BankIndex)
            engineA.objExtendedPalette.ptr = nullptr;
    }
}

// ------------------------------------------------------------------------------------------------------------------- G
template<>
void LCD::switchVramBank<6>(){
    const int BankIndex = 6; // G == 6
    auto &control = vramControl[BankIndex].state;

    const Byte ofs0 = control.ofs & 1u;
    const Byte ofs1 = (control.ofs >> 1) & 1u;

    if(control.mst == 0){
        const Word index = 512 + BANK_G_BEGIN;
        pageTable[index].state.bankID = BANK_G_BEGIN;
        pageTable[index].state.valid = true;
    }
    else if(control.mst == 1){
        const Word index = ofs0 + 4 * ofs1;
        pageTable[index].state.bankID = BANK_G_BEGIN;
        pageTable[index].state.valid = true;
    }
    else if(control.mst == 2){
        const Word index = 256 + ofs0 + 4 * ofs1;
        pageTable[index].state.bankID = BANK_G_BEGIN;
        pageTable[index].state.valid = true;
    }
    else if(control.mst == 3){
        const Word index = ofs0 + 4 * ofs1;
        engine3D.texturePalette[index] = {.ptr = vram.data() + BANK_G_BEGIN * 0x4000, .allocatedTo = BankIndex};
    }
    else if(control.mst == 4){
        const Word ofs = std::min(control.ofs, (Byte)1u);
        const Word index = ofs * 2; // 0 = 0..1, 1 = 2..3
        engineA.bgExtendedPalette[index] = {.ptr = vram.data() + BANK_G_BEGIN * 0x4000, .allocatedTo = BankIndex};
        engineA.bgExtendedPalette[index + 1] = {.ptr = vram.data() + BANK_G_BEGIN * 0x4000 + 0x2000, .allocatedTo = BankIndex};
    }
    else if(control.mst == 5){
        engineA.objExtendedPalette = {.ptr = vram.data() + BANK_G_BEGIN * 0x4000, .allocatedTo = BankIndex};
    }
}

template<>
void LCD::clearVramBank<6>(){
    const int BankIndex = 6; // G == 6
    auto &control = vramControl[BankIndex].state;

    const Byte ofs0 = control.ofs & 1u;
    const Byte ofs1 = (control.ofs >> 1) & 1u;

    if(control.mst == 0){
        const Word index = 512 + BANK_G_BEGIN;
        if(pageTable[index].state.bankID >= BANK_G_BEGIN && pageTable[index].state.bankID < BANK_G_END){
            pageTable[index].state.valid = false;
        }
    }
    else if(control.mst == 1){
        const Word index = ofs0 + 4 * ofs1;
        if(pageTable[index].state.bankID >= BANK_G_BEGIN && pageTable[index].state.bankID < BANK_G_END){
            pageTable[index].state.valid = false;
        }
    }
    else if(control.mst == 2){
        const Word index = 256 + ofs0 + 4 * ofs1;
        if(pageTable[index].state.bankID >= BANK_G_BEGIN && pageTable[index].state.bankID < BANK_G_END){
            pageTable[index].state.valid = false;
        }
    }
    else if(control.mst == 3){
        const Word index = ofs0 + 4 * ofs1;
        if(engine3D.texturePalette[index].allocatedTo == BankIndex)
            engine3D.texturePalette[index].ptr = nullptr;
    }
    else if(control.mst == 4){
        const Word ofs = std::min(control.ofs, (Byte)1u);
        const Word index = ofs * 2; // 0 = 0..1, 1 = 2..3
        if(engineA.bgExtendedPalette[index].allocatedTo == BankIndex)
            engineA.bgExtendedPalette[index].ptr = nullptr;
        if(engineA.bgExtendedPalette[index + 1].allocatedTo == BankIndex)
            engineA.bgExtendedPalette[index + 1].ptr = nullptr;
    }
    else if(control.mst == 5){
        if(engineA.objExtendedPalette.allocatedTo == BankIndex)
            engineA.objExtendedPalette.ptr = nullptr;
    }
}

// ------------------------------------------------------------------------------------------------------------------- H
template<>
void LCD::switchVramBank<8>(){
    const int BankIndex = 8; // H == 8
    auto &control = vramControl[BankIndex].state;
    control.mst &= 0b11; // "bit 2 not used"

    if(control.mst == 0){
        for(Byte i = 0; i < BANK_H_SIZE; i++){
            const Word index = 512 + BANK_H_BEGIN + i;
            pageTable[index].state.bankID = BANK_H_BEGIN + i;
            pageTable[index].state.valid = true;
        }
    }
    else if(control.mst == 1){
        for(Byte i = 0; i < BANK_H_SIZE; i++){
            const Word index = 128 + i;
            pageTable[index].state.bankID = BANK_H_BEGIN + i;
            pageTable[index].state.valid = true;
        }
    }
    else if(control.mst == 2){
        for(Byte i = 0; i < 4; i++){
            engineB.bgExtendedPalette[i] = {.ptr = vram.data() + BANK_H_BEGIN * 0x4000 + 0x2000 * i, .allocatedTo = BankIndex};
        }
    }
}

template<>
void LCD::clearVramBank<8>(){
    const int BankIndex = 8; // H == 8
    auto &control = vramControl[BankIndex].state;
    control.mst &= 0b11; // "bit 2 not used"

    if(control.mst == 0){
        for(Byte i = 0; i < BANK_H_SIZE; i++){
            const Word index = 512 + BANK_H_BEGIN + i;
            if(pageTable[index].state.bankID >= BANK_H_BEGIN && pageTable[index].state.bankID < BANK_H_END){
                pageTable[index].state.valid = false;
            }
        }
    }
    else if(control.mst == 1){
        for(Byte i = 0; i < BANK_H_SIZE; i++){
            const Word index = 128 + i;
            if(pageTable[index].state.bankID >= BANK_H_BEGIN && pageTable[index].state.bankID < BANK_H_END){
                pageTable[index].state.valid = false;
            }
        }
    }
    else if(control.mst == 2){
        for(Byte i = 0; i < 4; i++){
            if(engineB.bgExtendedPalette[i].allocatedTo == BankIndex)
                engineB.bgExtendedPalette[i].ptr = nullptr;
        }
    }
}

// ------------------------------------------------------------------------------------------------------------------- I
template<>
void LCD::switchVramBank<9>(){
    const int BankIndex = 9; // I == 9
    auto &control = vramControl[BankIndex].state;
    control.mst &= 0b11; // "bit 2 not used"

    if(control.mst == 0){
        const Word index = 512 + BANK_I_BEGIN;
        pageTable[index].state.bankID = BANK_I_BEGIN;
        pageTable[index].state.valid = true;
    }
    else if(control.mst == 1){
        const Word index = 130; // 0x6208000 = 128 + 2
        pageTable[index].state.bankID = BANK_I_BEGIN;
        pageTable[index].state.valid = true;
    }
    else if(control.mst == 2){
        const Word index = 384;
        pageTable[index].state.bankID = BANK_I_BEGIN;
        pageTable[index].state.valid = true;
    }
    else if(control.mst == 3){
        engineB.objExtendedPalette = {.ptr = vram.data() + BANK_I_BEGIN * 0x4000, .allocatedTo = BankIndex};
    }
}

template<>
void LCD::clearVramBank<9>(){
    const int BankIndex = 9; // I == 9
    auto &control = vramControl[BankIndex].state;
    control.mst &= 0b11; // "bit 2 not used"

    if(control.mst == 0){
        const Word index = 512 + BANK_I_BEGIN;
        if(pageTable[index].state.bankID >= BANK_I_BEGIN && pageTable[index].state.bankID < BANK_I_END){
            pageTable[index].state.valid = false;
        }
    }
    else if(control.mst == 1){
        const Word index = 130; // 0x6208000 = 128 + 2
        if(pageTable[index].state.bankID >= BANK_I_BEGIN && pageTable[index].state.bankID < BANK_I_END){
            pageTable[index].state.valid = false;
        }
    }
    else if(control.mst == 2){
        const Word index = 384;
        if(pageTable[index].state.bankID >= BANK_I_BEGIN && pageTable[index].state.bankID < BANK_I_END){
            pageTable[index].state.valid = false;
        }
    }
    else if(control.mst == 3){
        if(engineB.objExtendedPalette.allocatedTo == BankIndex)
            engineB.objExtendedPalette.ptr = nullptr;
    }
}
