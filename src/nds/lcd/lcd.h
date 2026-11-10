#pragma once
#include "../bus_types.h"
#include "2d/ppu.h"
#include "3d/gpu.h"
#include <thread>
#include "blockingconcurrentqueue.h"
#include <variant>
#include "readerwriterqueue.h"

namespace nds{
    class SharedBus;

    union PageTableEntry{
        struct{
            Byte bankID : 7; // nur Bank-Index aus VRAM (0-40) (41 Bänke) (41 * 16 * 1024) = 0xA4000 (656KB)
            Byte valid : 1; // 1 - gemappt, 0 - nicht gemappt
        } state;
        Byte raw = 0;
    };

    union VRAMCNT_T{
        struct{
            Byte mst : 3;
            Byte ofs : 2;
            Byte _unused : 2;
            Byte enable : 1;
        } state;
        Byte raw = 0;
    };

    union MASTERBRIGHT_T{
        struct{
            HalfWord factor : 5;
            HalfWord _unused : 9;
            HalfWord mode : 2;
        } state;
        HalfWord raw = 0;
    };

    struct MemoryWrite{
        Word addr = 0;
        Byte val = 0;
    };

    struct VRAMWrite{
        MemoryWrite write;
    };

    struct EngineCommand{
        PPU* engine;
        void (PPU::*function)();
    };

    struct PPUAllocationBGEvent{ // bg extended palette
        PPUAllocation allocation;
        PPU* engine;
        size_t index;
    };

    struct PPUAllocationOBJEvent{ // obj extended palette
        PPUAllocation allocation;
        PPU* engine;
    };

    enum PoisonPill : int{};

    typedef std::variant<
        MemoryWrite,
        VRAMWrite,
        EngineCommand,
        PoisonPill,
        PPUAllocationBGEvent,
        PPUAllocationOBJEvent
    > RenderEvent;

    enum Event3D{
        DESTROY_3D,
        RENDER_3D,
    };

    class LCD{

        SharedBus* bus;
        
        public:
        PPU engineA;
        PPU engineB;
        GPU engine3D;


        // 0x0600'0000 - 0x0700'0000 = 1024 * 16KB
        std::array<PageTableEntry, 1024> pageTable = {};
        std::array<PageTableEntry, 16> arm7PageTable = {};
        // ACHTUNG, [7] existiert nicht, [6] ist Bank G, [8] ist Bank H, [9] ist Bank I
        std::array<VRAMCNT_T, 10> vramControl = {};
        std::array<Byte, 0xA4000> vram = {};
        MASTERBRIGHT_T masterBright = {.raw = 0};
        
        std::array<Byte, 0xA4000> vramSync = {};
        // Achtung, enthält aktuell auch Register, die eigentlich nicht lesbar sind
        std::array<Byte, 0x6C> aRegisters = {};
        std::array<Byte, 0x6C> bRegisters = {};

        moodycamel::ReaderWriterQueue<RenderEvent> q;
        moodycamel::ReaderWriterQueue<Event3D> q3D;
        std::atomic<bool> workerNotify = false;
        std::atomic<bool> queueFrameFinished = false;
        std::atomic<bool> worker3DNotify = false;
        std::atomic<bool> queue3DFrameFinished = false;
        std::atomic<size_t> lineCount3D = 192;
        std::thread renderThread;
        std::thread render3DThread;

        void worker();
        void worker3D();

        private:
        template<Word N>
        void switchVramBank();
        template<Word N>
        void clearVramBank();
        
        LCDSTATUS_T DISPSTAT_7 = {.raw = 0}; // engine A + B
        LCDSTATUS_T DISPSTAT_9 = {.raw = 0}; // engine A + B
        std::atomic<bool> hasframe = false;
        
        public:
        bool swap3DBuffers = false;
        uint32_t* framebuffer;
        size_t framebufferSize = 0;
        uint32_t* backBuffer;
        Word vCount = 0; // engine A + B
        LCD(SharedBus* bus) : bus(bus), engine3D(bus){
            framebufferSize = 256*192*2;
            framebuffer = new uint32_t[framebufferSize]();
            backBuffer = new uint32_t[framebufferSize]();
            renderThread = std::thread(worker, this);
            render3DThread = std::thread(worker3D, this);
        };
        ~LCD(){
            q.enqueue(PoisonPill());
            q3D.enqueue(DESTROY_3D);
            workerNotify = true;
            worker3DNotify = true;
            workerNotify.notify_one();
            worker3DNotify.notify_one();
            if(renderThread.joinable()) renderThread.join();
            if(render3DThread.joinable()) render3DThread.join();
            delete[] framebuffer;
            delete[] backBuffer;
        }
        void init();
        void renderScanline();
        
        void onHBlank();
        void onVBlank();
        void increment();

        Word getVCount();

        bool hasFrame();

        Byte readRegister(Word addr, bool isNDS9);
        void writeRegister(Word addr, Byte val, bool isNDS9);
        void writeSync(Word addr, Byte val);
        Byte readMemory(Word addr);
        Byte readSyncedMemory(Word addr);
        Word getVramOffset(Word addr);
        void writeMemory(Word addr, Byte val);
        Byte readMemoryNDS7(Word addr);
        void writeMemoryNDS7(Word addr, Byte val);

        void writeVramBankControl(Word addr, Byte val);
        Byte readVramBankControl(Word addr);
        Byte getVRAMSTAT();

        uint32_t* accessFramebuffer();
    };
    
    template<> void LCD::switchVramBank<0>();
    template<> void LCD::switchVramBank<1>();
    template<> void LCD::switchVramBank<2>();
    template<> void LCD::switchVramBank<3>();
    template<> void LCD::switchVramBank<4>();
    template<> void LCD::switchVramBank<5>();
    template<> void LCD::switchVramBank<6>();
    template<> void LCD::switchVramBank<8>();
    template<> void LCD::switchVramBank<9>();
    template<> void LCD::clearVramBank<0>();
    template<> void LCD::clearVramBank<1>();
    template<> void LCD::clearVramBank<2>();
    template<> void LCD::clearVramBank<3>();
    template<> void LCD::clearVramBank<4>();
    template<> void LCD::clearVramBank<5>();
    template<> void LCD::clearVramBank<6>();
    template<> void LCD::clearVramBank<8>();
    template<> void LCD::clearVramBank<9>();
}
