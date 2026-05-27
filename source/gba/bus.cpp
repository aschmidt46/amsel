#include "bus.h"

using namespace gba;


// General Internal Memory
//   00000000-00003FFF   BIOS - System ROM         (16 KBytes)
//   00004000-01FFFFFF   Not used
//   02000000-0203FFFF   WRAM - On-board Work RAM  (256 KBytes) 2 Wait
//   02040000-02FFFFFF   Not used
//   03000000-03007FFF   WRAM - On-chip Work RAM   (32 KBytes)
//   03008000-03FFFFFF   Not used
//   04000000-040003FE   I/O Registers
//   04000400-04FFFFFF   Not used
// Internal Display Memory
//   05000000-050003FF   BG/OBJ Palette RAM        (1 Kbyte)
//   05000400-05FFFFFF   Not used
//   06000000-06017FFF   VRAM - Video RAM          (96 KBytes)
//   06018000-06FFFFFF   Not used
//   07000000-070003FF   OAM - OBJ Attributes      (1 Kbyte)
//   07000400-07FFFFFF   Not used
// External Memory (Game Pak)
//   08000000-09FFFFFF   Game Pak ROM/FlashROM (max 32MB) - Wait State 0
//   0A000000-0BFFFFFF   Game Pak ROM/FlashROM (max 32MB) - Wait State 1
//   0C000000-0DFFFFFF   Game Pak ROM/FlashROM (max 32MB) - Wait State 2
//   0E000000-0E00FFFF   Game Pak SRAM    (max 64 KBytes) - 8bit Bus width
//   0E010000-0FFFFFFF   Not used
// Unused Memory Area
//   10000000-FFFFFFFF   Not used (upper 4bits of address bus unused)

Byte* gba::Bus::accessMemory(Word addr)
{
    (void)addr;
    return nullptr;
}

void gba::Bus::writeByte(Word addr, Byte val)
{
    (void)addr;
    (void)val;
}

Byte gba::Bus::readByte(Word addr)
{
    (void)addr;
    return Byte();
}

void gba::Bus::writeHalfWord(Word addr, HalfWord val)
{
    writeByte(addr, val & 0xFF);
    writeByte(addr + 1, (val & (0xFF << 8)) >> 8);
}

HalfWord gba::Bus::readHalfWord(Word addr)
{
    HalfWord A1 = readByte(addr);
    HalfWord A2 = readByte(addr + 1);
    return A1 | (A2 << 8);
}

void gba::Bus::writeWord(Word addr, Word val)
{
    writeByte(addr, val & 0xFF);
    writeByte(addr + 1, (val & (0xFF << 8)) >> 8);
    writeByte(addr + 2, (val & (0xFF << 16)) >> 16);
    writeByte(addr + 3, (val & (0xFF << 24)) >> 24);
}

Word gba::Bus::readWord(Word addr)
{
    Word A1 = readByte(addr);
    Word A2 = readByte(addr + 1);
    Word A3 = readByte(addr + 2);
    Word A4 = readByte(addr + 3);
    return A1 | (A2 << 8) | (A3 << 16) | (A4 << 24);
}
