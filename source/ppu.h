#pragma once
#include <cstdint>
#include "screen.h"
#include <coroutine>
#include "mapper.h"

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

struct [[gnu::packed]] OAMSprite{
    uint8_t yPos;   // Top of sprite + 1
    uint8_t tileIndex;
    uint8_t attributes;
    uint8_t xPos;
};

static_assert(sizeof(OAMSprite) == 4, "OAMSprite hat falsche Größe!");

class Screen;
class Mapper;
class Ppu{
    private:
    uint16_t v = 0;
    uint16_t t = 0;
    uint8_t x = 0;
    bool w = false;
    
    public:

    // Extern
    uint8_t PPUCTRL = 0;
    uint8_t PPUMASK = 0;
    uint8_t PPUSTATUS = 0x3D;
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

    Ppu() : state(frame()) {
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

    int currentScanline = 261; //262 insg
    int currentDot; //242
    bool blanking = true;
    bool frameReady = false;

    FrameRoutine frame();
    FrameRoutine state;
    bool unevenFrame = true;
    void clock();
    void setPixel(int x, int y, glm::vec3 c);

    // Callbacks
    void wroteRegister(uint8_t* reg);
    void readRegister(uint8_t* reg);

    //Registergetter / setter
    void setVBlank(bool val);
    void setSpriteZero(bool val);
    void setOverflow(bool val);
    bool getVBlank();
    bool getNMIOutput();
    bool getVRAMAddressIncrement(); //1 oder 32
    void incrementX();
    void incrementFineX();
    void incrementY();
    bool isRenderingEnabled();
    glm::vec3 getColor(int index);
};



