#pragma once
#include "../../bus_types.h"
#include "gpu_types.h"
#include <vector>
#include <stack>
#include <queue>
#include <atomic>

namespace nds{
    class SharedBus;
    struct PIXEL_T;
    // ACHTUNG, aktuell setze ich GXFIFO IRQ nur einmal, nicht durchgehend (bleibt gesetzt, bis Bedingung falsch)
    // Idee: gpu_geometry.cpp und gpu_rendering.cpp, Threading später
    struct GPUSlotAllocation{
        Byte* ptr = nullptr;
        int allocatedTo = 0;
    };
    class GPU{

        size_t& getSP();
        std::array<mat4x4, 32>& getStack();

        struct { // alle stacks haben ZUSÄTZLICH noch eine "current" matrix
            size_t projectionSP = 0;
            size_t coordinateSP = 0; // auch für vector
            size_t textureSP = 0;
            std::array<mat4x4, 32> projection = {};
            std::array<mat4x4, 32> coordinate = {}; // aka position
            std::array<mat4x4, 32> directional = {}; // aka vector
            std::array<mat4x4, 32> texture = {};

            mat4x4 currentProjection = mat4x4::identity();
            mat4x4 currentCoordinate = mat4x4::identity();
            mat4x4 currentDirection = mat4x4::identity();
            mat4x4 currentTexture = mat4x4::identity();

            mat4x4* current = &currentProjection;
        } matrixStack;

        // Zustand Geometrie engine
        struct {
            bool manualTranslucentPolygonYSorting = false; 
            bool depthBufferingUseW = false; // sonst z
            int mtxMode = 0;
            POLYGON_ATTR_T nextPolygonAttr = {.raw=0};
            POLYGON_ATTR_T polygonAttr = {.raw=0};
            int primitiveType = 0;
            vec4 p; // letzter Vertex
            union{
                struct{
                    HalfWord red : 5;
                    HalfWord green : 5;
                    HalfWord blue : 5;
                    HalfWord _unused : 1;
                } state;
                HalfWord raw;
            } colour; // direkt
            int16_t nextS = 0;
            int16_t nextT = 0;
            TEXIMAGE_PARAM_T currentTexParam = {.raw = 0};
            Word paletteBaseAddress = 0;
        } geometry;

        inline uint16_t circularIncrement(const uint16_t cur, const Polygon &p, int inc) const{
            return (cur + inc + p.numVertices) % p.numVertices;
        }
        inline void maybeAdvanceEdgeWalk(const std::vector<Vertex> &vram, Polygon &p, const int currentY){
            while(currentY >= vram[p.vertices[p.nextL]].screenY && p.curL != p.bottom){
                p.curL = p.nextL;
                p.nextL = circularIncrement(p.curL, p, -1);
            }

            while(currentY >= vram[p.vertices[p.nextR]].screenY && p.curR != p.bottom){
                p.curR = p.nextR;
                p.nextR = circularIncrement(p.curR, p, 1);
            }
        }

        uint32_t getColour(const Polygon &p, const std::array<int32_t, 6> &attribs);

        struct {
            std::array<int32_t, 6> attributesL;
            std::array<int32_t, 6> attributesR;
            std::array<int32_t, 6> attributesPixel;
        } render;

        union {
            struct{
                Word x1 : 8;
                Word y1 : 8;
                Word x2 : 8;
                Word y2 : 8;
            } state;
            Word raw = 0;
        } viewport;
        int viewportTop = 0;
        int viewportBottom = 191;

        SharedBus* bus = nullptr;

        // sind bereits transformiert
        std::vector<Vertex> vertexRam;
        std::vector<Polygon> polyRam;
        int vertexIndex = 0;
        Polygon currentPolygon = {};
        std::array<Vertex, 10> currentPolyVertices = {};
        std::array<uint16_t, 10> currentPolyIndices = {};
        size_t triangleCounter = 0;
        void addVertex(const Vertex &v);
        bool polygonInViewport(const Polygon &p);
        void pushPolygon();
        std::vector<Vertex> vertexRamRender;
        std::vector<Polygon> polyRamRender;

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
        mat4x4 clipMtxResult;
        void recalculateClipMtx();
        mat4x4 vecMtxResult;

        // Hilfe für Cmds, adresse startet bei 0x04000400
        std::array<GXCMD, 0x200> addrToCmd = {};
        // Cmd index -> Anzahl Parameter
        std::array<size_t, 0x100> cmdToNumParams = {};
        std::array<void (GPU::*)(const FIFOCommand&), 0x100> cmdToFunction = {};



        std::queue<FIFOCommand> commandFifo;
        size_t commandFifoSize = 0;
        Word gxFifo = 0;
        int paramsRemaining = -1;
        int paramIndex = 0;
        int regParamsRemaining = -1;
        int paramRegIndex = 0;
        FIFOCommand buildingCommand{};
        FIFOCommand buildingRegCommand{};
        
        
        void updateIF();
        void fillMaps();
        
        GXSTAT_T GXSTAT = {.raw = 0};
        DISP3DCNT_T DISP3DCNT = {.raw = 0};
        
        public:
        std::array<std::atomic<uint32_t>, 256 * 192> framebuffer = {};
        bool haltGeometry = false;
        GPU(SharedBus* bus) : bus(bus){
            vertexRam.reserve(6144);
            vertexRamRender.reserve(6144);
            cmdToFunction.fill(&GPU::NOP);
            fillMaps();
        }
        ~GPU(){
        }
        std::array<GPUSlotAllocation, 4> textureRearPlane = {{nullptr, 0}}; // Jeweils 128KB
        std::array<GPUSlotAllocation, 6> texturePalette = {{nullptr, 0}}; // Jeweils 16KB
        void swapBuffers();
        
        void writeRegister(Word addr, Byte val);
        Byte readRegister(Word addr);
        void writeGXFIFO(Word val);
        void writeCommandRegister(Word addr, Word val);

        void renderScanline(const int y);

        void clockGeometry();
        void transformVertex(Vertex &v);
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
