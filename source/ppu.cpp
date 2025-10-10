#include "ppu.h"
#include <iostream>


// Coroutine?
// Gesamten Frame in diese Funktion?
FrameRoutine Ppu::frame()
{
    // Pre-render scanline

    // 341 dots pro scanline, außer in der Pre-Render scanline bei ungerader Frameanzahl
    int dotsPerLine = 341;
    for(int i = 0; i < dotsPerLine; i++){
        if(i==1){ setVBlank(false); blanking = false; setSpriteZero(false); setOverflow(false);}
        if(i>= 256) OAMADDR = 0;
        if(i>=279 && i <=303){
            // Vertikale Bits aus t nach v kopieren
            // Unten andersherum!
            v = v & 0b0000010000011111;
            v = v | (t & 0b0111101111100000);
        }
        co_await std::suspend_always{};
    }

    // Sichtbare Scanlines
    for(int i = 0; i <= 239; i++){

        // Idle-Zyklus
        if(i!=0 || unevenFrame)
            co_await std::suspend_always{};
        else{
            // bg isbit
        }

        // Zyklen 1-256
        for(int j = 0; j <= 255; j++){
            if(j % 8 == 0 && isRenderingEnabled()){
                uint16_t nametable_start = 0x2000;
                // Nametable Byte
                // uint16_t tile_address = 0x2000 | (v & 0x0FFF);
                uint16_t tile_address = nametable_start + (32 * (i / 8)) + (j / 8);
                uint8_t nt_index = mapper->readVRAM((uint8_t*)(uintptr_t)tile_address);
                uintptr_t nt_entry = (((uintptr_t)(nt_index)) * 16) + (i%8);
                // Attribute Table Byte
                // uint16_t attribute_address = 0x23C0 | (v & 0x0C00) | ((v >> 4) & 0x38) | ((v >> 2) & 0x07);
                uint16_t attribute_address = nametable_start + 0x3C0 + (8 * (i / 32)) + (j / 32);
                uint8_t at_entry = mapper->readVRAM((uint8_t*)(uintptr_t)attribute_address);
                uint8_t att_bits;
                //std::cout << "slow" << std::endl;
                bool bottom = false, left = false;
                if(i%32 <16) bottom = true;
                if(j%32 <16) left = true;
                if(!bottom && left) att_bits = (at_entry & 0b00000011) << 2; // top left
                if(!bottom && !left) att_bits = (at_entry & 0b00001100); // top right
                if(bottom && left) att_bits = (at_entry & 0b00110000) >> 2; // bottom left
                if(bottom && !left) att_bits = (at_entry & 0b11000000) >> 4; // bottom right
                // Niedriges Pattern Table Tile
                uint8_t pt_entry_plane_1 = mapper->readVRAM((uint8_t*)backgroundTable + nt_entry);
                // Hohes Pattern Table Tile (+8 bytes vom ersten)
                uint8_t pt_entry_plane_2 = mapper->readVRAM((uint8_t*)(backgroundTable + nt_entry+8));
                for(int g = 0; g < 8; g++){
                    bool p1 = pt_entry_plane_1 & (128u >> g);
                    bool p2 = pt_entry_plane_2 & (128u >> g);
                    uint8_t color_index;
                    if(p1 && p2) color_index = 3;
                    else if(p2) color_index = 2;
                    else if(p1) color_index = 1;
                    else color_index = 0;
                    //auto color = getColor(color_index); // In echt dann aus attribute index
                    color_index = color_index | att_bits | 0b00010000; // Hintergrund
                    uint8_t pallete_value = mapper->readVRAM((uint8_t*)(uintptr_t)0x3F00 + color_index);
                    setPixel(j + g, i, pal.getColor(pallete_value));
                }
                // Das braucht 8 dots und generiert 8 Pixel
                incrementX();
            }
            if(j==256){
                // WRAPPING AROUND an dieser Stelle
                if(isRenderingEnabled()) incrementY();
            }
            co_await std::suspend_always{};
        }

        // Zyklen 257-320 (Tile-Daten für nächste Zeile fetchen)
        for(int j = 257; j <= 320; j++){
            if(j==257){
                // Horizontale Position aus t nach v kopieren;
                v = v & 0b0111101111100000;
                v = v | (t & 0b0000010000011111);
            }
            OAMADDR = 0;
            co_await std::suspend_always{};
        }

        // Zyklen 321-336
        for(int j = 321; j <= 336; j++){
            // beginnt bereits bei dot 328
            if((j == 328 || j == 236) && isRenderingEnabled()) incrementX();
            co_await std::suspend_always{};
        }

        // Zyklen 337-340
        for(int j = 337; j <= 340; j++){
            co_await std::suspend_always{};
        }
    }

    // Post-Render Scanline (Idle)
    for(int i = 0; i < dotsPerLine; i++){
        co_await std::suspend_always{};
    }

    // Vertical Blanking (241-260)
    for(int i = 241; i <= 259; i++){
        setVBlank(true);
        blanking = true;
        if(getVBlank() && getNMIOutput()) mapper->pullNMI();
        for(int i = 0; i < dotsPerLine; i++){
            co_await std::suspend_always{};
        }   
    }
    // Für die letzte Zeile von VBlank -1, damit im letzten dot die Coroutine beendet wird
    for(int i = 0; i < dotsPerLine-1; i++){
        co_await std::suspend_always{};
    }   

    co_return;
}

void Ppu::clock()
{
    [[unlikely]] if(!state.resume()) {
        // Frame fertig-gerendert
        state = frame();
        frameReady = true;
        unevenFrame = !unevenFrame;
        screen->copyBufferToScreen(pixelBuffer);
    }
}

void Ppu::setPixel(int x, int y, glm::vec3 c)
{
    int index = (3*x) + (3*256*y);
    pixelBuffer[index] = c.r;
    pixelBuffer[index + 1] = c.g;
    pixelBuffer[index + 2] = c.b;
}

void Ppu::setVBlank(bool val)
{
    if(val){
        PPUSTATUS = PPUSTATUS | ((uint8_t)1u << 7);
    }
    else{
        PPUSTATUS = PPUSTATUS & ~((uint8_t)1u << 7);
    }
}

void Ppu::setSpriteZero(bool val)
{
    if(val){
        PPUSTATUS = PPUSTATUS | ((uint8_t)1u << 6);
    }
    else{
        PPUSTATUS = PPUSTATUS & ~((uint8_t)1u << 6);
    }
}

void Ppu::setOverflow(bool val)
{
    if(val){
        PPUSTATUS = PPUSTATUS | ((uint8_t)1u << 5);
    }
    else{
        PPUSTATUS = PPUSTATUS & ~((uint8_t)1u << 5);
    }
}

bool Ppu::getVBlank()
{
    return PPUSTATUS & 0b10000000;
}

bool Ppu::getNMIOutput()
{
    return PPUCTRL & 0b10000000;
}

bool Ppu::getVRAMAddressIncrement()
{
    return PPUCTRL & 0b00000100;
}

// Pseudocode von nesdev.org
void Ppu::incrementX()
{
    if ((v & 0x001F) == 31){    // if coarse X == 31
        v &= ~0x001F;           // coarse X = 0
        v ^= 0x0400;            // switch horizontal nametable
    }       
    else
        v += 1;                 // increment coarse X
}

void Ppu::incrementFineX()
{
    if(x < 7)
        x++;
    else x = 0;
}

// Pseudocode von nesdev.org
void Ppu::incrementY()
{

    if ((v & 0x7000) != 0x7000){        // if fine Y < 7
        v += 0x1000;                      // increment fine Y
    }
    else{
        v &= ~0x7000;                     // fine Y = 0
        int y = (v & 0x03E0) >> 5;        // let y = coarse Y
        if (y == 29){
          y = 0;                          // coarse Y = 0
          v ^= 0x0800;                    // switch vertical nametable
        }
        else if (y == 31){
          y = 0;                          // coarse Y = 0, nametable not switched
        }
        else{
          y += 1;                         // increment coarse Y
        }
        v = (v & ~0x03E0) | (y << 5);     // put coarse Y back into v
    }
}

bool Ppu::isRenderingEnabled()
{
    // Hintergrund- oder Spriterendering aktiv
    return ((PPUMASK & 0b00010000) || (PPUMASK & 0b00001000));
}

glm::vec3 Ppu::getColor(int index)
{
    switch(index){
        case 1:
            return glm::vec3(1,0,0);
        case 2:
            return glm::vec3(0,1,0);
        case 3:
            return glm::vec3(0,0,1);
        default:
            return glm::vec3(0,0,0);
    }
}

void Ppu::wroteRegister(uint8_t *reg)
{
    if(reg==&PPUCTRL){
        t = t & 0b1111001111111111;
        t = t | ((PPUCTRL << 10) & 0b0000110000000000);

        // Pattern table select Hintergrund
        bool second = PPUCTRL & 0b00010000;
        if(second) backgroundTable = 0x1000;
        else backgroundTable = 0;

    }
    else if(reg==&PPUMASK){

    }
    else if(reg==&PPUSTATUS){

    }
    else if(reg==&OAMADDR){

    }
    else if(reg==&OAMDATA){
        if(blanking){
            OAM[OAMADDR] = OAMDATA;
            OAMADDR++;
        }
    }
    else if(reg==&PPUSCROLL){
        if(!w){
            t = (t & 0b1111111111100000) | ((PPUSCROLL >> 3) & 0b00011111);
            x = PPUSCROLL & 0b00000111;
            w = true;
        }
        else{
            // WTF man
            uint16_t FGH = ((uint16_t)PPUSCROLL & 0b0000000000000111) << 12; // bit 0-2 von PPUSCROLL auf bit 14-12
            uint16_t ABCDE = ((uint16_t)PPUSCROLL & 0b0000000011111000) << 2; // bit 3-7 auf bit 5-9
            t = t & 0b0000110000011111;
            t = t | FGH | ABCDE;
            w = false;
        }
    }
    else if(reg==&PPUADDR){
        if(!w){
            t = (((uint16_t)PPUADDR) << 8);// & 0b0011111100000000;
            //v = t | (v & 0b0000000011111111);
            w = true;
        }
        else{
            t = t | ((uint16_t)PPUADDR);
            v = t;
            w = false;
        }
    }
    else if(reg==&PPUDATA){
        mapper->writeVRAM((uint8_t*)(uintptr_t)v, PPUDATA);
        if(getVRAMAddressIncrement()){
            v += 32;
        }
        else v += 1;
    }
}

void Ppu::readRegister(uint8_t *reg)
{
    if(reg==&PPUCTRL){

    }
    else if(reg==&PPUMASK){

    }
    else if(reg==&PPUSTATUS){
        w = false;
        setVBlank(false);
    }
    else if(reg==&OAMADDR){

    }
    else if(reg==&OAMDATA){

    }
    else if(reg==&PPUSCROLL){

    }
    else if(reg==&PPUADDR){
       
    }
    else if(reg==&PPUDATA){
        PPUDATA = mapper->readVRAM((uint8_t*)(uintptr_t)v);
    }
}
