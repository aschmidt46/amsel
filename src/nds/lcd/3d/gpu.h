#pragma once
#include "../../bus_types.h"
#include "gpu_types.h"
#include <vector>
#include <glm/glm.hpp>
#include <stack>
#include <queue>

namespace nds{
    class SharedBus;
    // ACHTUNG, aktuell setze ich GXFIFO IRQ nur einmal, nicht durchgehend (bleibt gesetzt, bis Bedingung falsch)
    // Idee: gpu_geometry.cpp und gpu_rendering.cpp, Threading später
    struct GPUSlotAllocation{
        Byte* ptr = nullptr;
        int allocatedTo = 0;
    };
    class GPU{

        struct {
            size_t projectionSP = 0;
            size_t coordinateSP = 0; // auch für vector
            size_t textureSP = 0;
            std::array<glm::i32mat4x4, 1> projection = {};
            std::array<glm::i32mat4x4, 32> coordinate = {}; // aka position
            std::array<glm::i32mat4x4, 32> directional = {}; // aka vector
            glm::i32mat4x4 texture;
        } matrixStack;

        bool fifoHalted = false; // von Swapbuffers gesetzt
        // Zustand Geometrie engine
        struct {

        } geometry;

        union {
            struct{
                Word x1 : 8;
                Word y1 : 8;
                Word x2 : 8;
                Word y2 : 8;
            } state;
            Word raw = 0;
        } viewport;

        SharedBus* bus = nullptr;

        // sind bereits transformiert
        std::vector<glm::ivec4> vertexPolyRam;
        std::vector<glm::ivec4> vertexPolyRamBack;

        std::array<Byte, 128> shininess = {};
        std::array<Byte, 0x10> edgeColor = {};
        std::array<Byte, 0x20> fogTable = {};
        std::array<Byte, 0x40> toonTable = {};
        Byte alphaTestRef = 0;
        Word clearColor = 0;
        HalfWord clearDepth = 0;
        HalfWord clrImageOffset = 0;
        Word fogColor = 0;
        HalfWord fogOffset = 0;
        HalfWord disp1DotDepth = 0;
        std::array<Byte, 0x10> posResult = {};
        std::array<Byte, 6> vecResult = {};
        glm::i32mat4x4 clipMtxResult;
        glm::i32mat4x4 vecMtxResult;

        // Hilfe für Cmds, adresse startet bei 0x04000400
        std::array<GXCMD, 0x200> addrToCmd = {};
        // Cmd index -> Anzahl Parameter
        std::array<size_t, 0x80> cmdToNumParams = {};
        std::array<void (GPU::*)(const FIFOCommand&), 0x80> cmdToFunction = {};



        std::queue<FIFOCommand> commandFifo;
        Word gxFifo = 0;
        int paramsRemaining = -1;
        int paramIndex = 0;
        FIFOCommand buildingCommand{};

        inline void swapBuffers(){
            vertexPolyRam.swap(vertexPolyRamBack);
        }

        void updateIF();
        void fillMaps();

        GXSTAT_T GXSTAT = {.raw = 0};
        DISP3DCNT_T DISP3DCNT = {.raw = 0};

        public:
        GPU(SharedBus* bus) : bus(bus){
            vertexPolyRam.reserve(100000);
            vertexPolyRamBack.reserve(100000);
            cmdToFunction.fill(&GPU::NOP);
            fillMaps();
        }
        ~GPU(){
        }
        std::array<GPUSlotAllocation, 4> textureRearPlane = {{nullptr, 0}}; // Jeweils 128KB
        std::array<GPUSlotAllocation, 6> texturePalette = {{nullptr, 0}}; // Jeweils 16KB

        void writeRegister(Word addr, Byte val);
        Byte readRegister(Word addr);
        void writeGXFIFO(Word val);
        void writeCommandRegister(Word addr, Word val);

        void clockGeometry();
        void NOP(const FIFOCommand &cmd);
        void MTX_MODE(const FIFOCommand &cmd);
        void MTX_PUSH(const FIFOCommand &cmd);
        void MTX_POP(const FIFOCommand &cmd);
        void MTX_STORE(const FIFOCommand &cmd);
        void MTX_RESTORE(const FIFOCommand &cmd);
        void MTX_IDENTITY(const FIFOCommand &cmd);
        void MTX_LOAD_4x4(const FIFOCommand &cmd);
        void MTX_LOAD_4x3(const FIFOCommand &cmd);
        void MTX_MULT_4x4(const FIFOCommand &cmd);
        void MTX_MULT_4x3(const FIFOCommand &cmd);
        void MTX_MULT_3x3(const FIFOCommand &cmd);
        void MTX_SCALE(const FIFOCommand &cmd);
        void MTX_TRANS(const FIFOCommand &cmd);
        void COLOR(const FIFOCommand &cmd);
        void NORMAL(const FIFOCommand &cmd);
        void TEXCOORD(const FIFOCommand &cmd);
        void VTX_16(const FIFOCommand &cmd);
        void VTX_10(const FIFOCommand &cmd);
        void VTX_XY(const FIFOCommand &cmd);
        void VTX_XZ(const FIFOCommand &cmd);
        void VTX_YZ(const FIFOCommand &cmd);
        void VTX_DIFF(const FIFOCommand &cmd);
        void POLYGON_ATTR(const FIFOCommand &cmd);
        void TEXIMAGE_PARAM(const FIFOCommand &cmd);
        void PLTT_BASE(const FIFOCommand &cmd);
        void DIF_AMB(const FIFOCommand &cmd);
        void SPE_EMI(const FIFOCommand &cmd);
        void LIGHT_VECTOR(const FIFOCommand &cmd);
        void LIGHT_COLOR(const FIFOCommand &cmd);
        void SHININESS(const FIFOCommand &cmd);
        void BEGIN_VTXS(const FIFOCommand &cmd);
        void END_VTXS(const FIFOCommand &cmd);
        void SWAP_BUFFERS(const FIFOCommand &cmd);
        void VIEWPORT(const FIFOCommand &cmd);
        void BOX_TEST(const FIFOCommand &cmd);
        void POS_TEST(const FIFOCommand &cmd);
        void VEC_TEST(const FIFOCommand &cmd);
    };
}
