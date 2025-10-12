#pragma once
#include <cstdint>
#include "screen.h"
#include <coroutine>
#include "mapper.h"
#include "palette.h"
#include <queue>
#include <memory>

// Die Coroutinen-Klasse
struct FrameRoutine
{
   struct promise_type; // Vorwaertsdeklaration
   using handle = std::coroutine_handle<promise_type>;
   // promise_type Klasse
   struct promise_type
   {
      // Aufruf unmittelbar nach Start der Coroutine
      // Soll die Coroutine zu Beginn nicht unterbrochen werden,
      // ist anstelle eines suspend_always-Objekts ein Objekt vom Typ suspend_never zurückzugeben.
      auto initial_suspend()
      { return std::suspend_always{}; }

      // Aufruf nach Beenden der Coroutine
      auto final_suspend() noexcept
      { return std::suspend_always{}; }

      // Aufruf beim Ausloesen einer nicht behandelten
      // Ausnahme
      void unhandled_exception()
     { std::terminate(); }

      // Liefert das Coroutinen-Objekt zurueck
      auto get_return_object()
     { return FrameRoutine{handle::from_promise(*this)}; }

      // Wird durch co_return aufgerufen
     void return_void()
     { }
   };

   // Setzt Coroutine fort (coro.resume()),
   // wenn das Coroutinen-Handle gueltig ist (ungleich 0)
   bool resume()
   { return coro ? (coro.resume(), !coro.done()) : false; };

 private:
   // Coroutinen-Handle
   handle coro;
   // ctor, speichert das uebergebene Coroutinen-Handle ab
   FrameRoutine(handle h) : coro(h) {}
};


union loopy_register {
    uint16_t value = 0x0000;
    struct{
        // Coarse X (tile column) (Bits 0-4)
        uint16_t coarse_x : 5;

        // Coarse Y (tile row) (Bits 5-9)
        uint16_t coarse_y : 5;

        // Nametable select (Bits 10-11)
        uint16_t name_table_x : 1;
        uint16_t name_table_y : 1;

        // Fine Y offset (Vertical offset within a tile) (Bits 12-14)
        uint16_t fine_y : 3;
        uint16_t unused : 1;
    };
};

union ctrlreg {
    uint8_t value;
    struct{
        uint8_t nametable_x : 1;
        uint8_t nametable_y : 1;
        uint8_t increment_mode : 1;
        uint8_t pattern_sprite : 1;
        uint8_t pattern_background : 1;
        uint8_t sprite_size : 1;
        uint8_t unused : 1;
        uint8_t enable_nmi : 1;
    };
};

union maskreg
	{
        uint8_t reg;
		struct
		{
			uint8_t grayscale : 1;
			uint8_t render_background_left : 1;
			uint8_t render_sprites_left : 1;
			uint8_t render_background : 1;
			uint8_t render_sprites : 1;
			uint8_t enhance_red : 1;
			uint8_t enhance_green : 1;
			uint8_t enhance_blue : 1;
		};

	};

    union statusreg
	{
		struct
		{
			uint8_t unused : 5;
			uint8_t sprite_overflow : 1;
			uint8_t sprite_zero_hit : 1;
			uint8_t vertical_blank : 1;
		};

		uint8_t reg;
	};

class Screen;
class Mapper;
class Ppu{
    private:
    loopy_register vram_addr;
    loopy_register tram_addr;
    uint8_t fine_x = 0;
    bool w = false;
    
    public:

    // Extern
    ctrlreg PPUCTRL;
    maskreg mask;
    statusreg PPUSTATUS;
    uint8_t OAMADDR = 0;
    uint8_t OAMDATA = 0;
    uint8_t PPUSCROLL = 0;
    uint8_t PPUADDR = 0;
    uint8_t PPUDATA = 0;

    uint8_t* internalMemory; // 2KB
    uint8_t* palletteIndexes; // 0x0020 Bytes
    uint8_t* OAM; // 256 Bytes (64 * 4)

    

    Screen* screen;
    Mapper* mapper;

    // Output, nicht Teil der PPU
    float* pixelBuffer;

    Palette pal;

    uint8_t vram_buffer;

    uint8_t bg_next_tile_id = 0x00;
    uint8_t bg_next_tile_attrib = 0x00;
    uint8_t bg_next_tile_lsb = 0x00;
    uint8_t bg_next_tile_msb = 0x00;

    uint16_t bg_shifter_pattern_lo = 0x0000;
	uint16_t bg_shifter_pattern_hi = 0x0000;
	uint16_t bg_shifter_attrib_lo  = 0x0000;
	uint16_t bg_shifter_attrib_hi  = 0x0000;


    Ppu() : pal("palette.pal"), state(fakeFrame()) {
        pixelBuffer = new float[256*240*3];
        for(int i = 0; i < 256*240*3; i++){
            pixelBuffer[i] = 0;
        }
        internalMemory = new uint8_t[0x0800];
        palletteIndexes = new uint8_t[0x0020];
        OAM = new uint8_t[256];
    };
    ~Ppu(){
        delete[] pixelBuffer;
        delete[] internalMemory;
        delete[] palletteIndexes;
        delete[] OAM;
    };
    void init(Mapper* m, Screen* s){
        mapper = m;
        screen = s;
    };

    bool blanking = true;
    bool frameReady = false;

    // Pixel "dot" position information
	int16_t scanline = 0;
	int16_t cycle = 0;

    FrameRoutine state;

    uintptr_t backgroundTable = 0x1000;

    bool unevenFrame = true;
    void clock();
    FrameRoutine fakeFrame();
    void fakeClock();
    void setPixel(int x, int y, glm::vec3 c);

    // Callbacks
    void writeRegister(uint8_t* reg, uint8_t val);
    uint8_t readRegister(uint8_t* reg);

};



