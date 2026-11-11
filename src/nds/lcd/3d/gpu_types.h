#pragma once
#include "../../bus_types.h"
#include <array>
#include "matrix_math.h"
#include <variant>

namespace nds{

    union DISP3DCNT_T{
        struct {
            HalfWord enableTextureMapping : 1;
            HalfWord polygonAttrShading : 1;
            HalfWord enableAlphaTest : 1;
            HalfWord enableAlphaBlending : 1;
            HalfWord enableAntiAliasing : 1;
            HalfWord enableEdgeMarking : 1;
            HalfWord fogColor : 1;
            HalfWord fogMasterEnable : 1;
            HalfWord fogDepthShift : 4;
            HalfWord colorBufferUnderflow : 1;
            HalfWord polygonVertexRamOverflow : 1;
            HalfWord rearPlaneModeBitmap : 1;
            HalfWord _unused : 1;
        } state;
        HalfWord raw;
    };

    union GXSTAT_T{
        struct{
            Word boxPosVecTestBusy : 1;
            Word boxTestResult : 1;
            Word _unused0 : 6;
            Word posVecMatrixStackLevel : 5;
            Word projectionMatrixStackLevel : 1;
            Word matrixStackBusy : 1;
            Word matrixStackOverUnderflow : 1;
            Word commandFifoSize40bitentries : 9;
            Word commandFifoLessThanHalfFull : 1;
            Word commandFifoEmpty : 1;
            Word geometryEngineBusy : 1;
            Word _unused1 : 2;
            Word commandFifoIRQ : 2;
        } state;
        Word raw;
    };

    constexpr Word gxStatWriteMask = 0xC0000000; // nur IRQ bits


    enum GXCMD : Byte{
        GXCMD_NOP = 0x0,
        GXCMD_MTX_MODE = 0x10,
        GXCMD_MTX_PUSH = 0x11,
        GXCMD_MTX_POP = 0x12,
        GXCMD_MTX_STORE = 0x13,
        GXCMD_MTX_RESTORE = 0x14,
        GXCMD_MTX_IDENTITY = 0x15,
        GXCMD_MTX_LOAD_4x4 = 0x16,
        GXCMD_MTX_LOAD_4x3 = 0x17,
        GXCMD_MTX_MULT_4x4 = 0x18,
        GXCMD_MTX_MULT_4x3 = 0x19,
        GXCMD_MTX_MULT_3x3 = 0x1A,
        GXCMD_MTX_SCALE = 0x1B,
        GXCMD_MTX_TRANS = 0x1C,
        GXCMD_COLOR = 0x20,
        GXCMD_NORMAL = 0x21,
        GXCMD_TEXCOORD = 0x22,
        GXCMD_VTX_16 = 0x23,
        GXCMD_VTX_10 = 0x24,
        GXCMD_VTX_XY = 0x25,
        GXCMD_VTX_XZ = 0x26,
        GXCMD_VTX_YZ = 0x27,
        GXCMD_VTX_DIFF = 0x28,
        GXCMD_POLYGON_ATTR = 0x29,
        GXCMD_TEXIMAGE_PARAM = 0x2A,
        GXCMD_PLTT_BASE = 0x2B,
        GXCMD_DIF_AMB = 0x30,
        GXCMD_SPE_EMI = 0x31,
        GXCMD_LIGHT_VECTOR = 0x32,
        GXCMD_LIGHT_COLOR = 0x33,
        GXCMD_SHININESS = 0x34,
        GXCMD_BEGIN_VTXS = 0x40,
        GXCMD_END_VTXS = 0x41,
        GXCMD_SWAP_BUFFERS = 0x50,
        GXCMD_VIEWPORT = 0x60,
        GXCMD_BOX_TEST = 0x70,
        GXCMD_POS_TEST = 0x71,
        GXCMD_VEC_TEST = 0x72,
    };

    struct FIFOCommand{
        GXCMD cmd = GXCMD_NOP;
        std::array<Word, 32> params = {};
    };

    union POLYGON_ATTR_T{
        struct{
            Word light : 4;
            Word polygonMode : 2;
            Word renderPolyBackside : 1; // Liniensegmente IMMER
            Word renderPolyFrontside : 1;
            Word _unused0 : 3;
            Word depthTranslucentSetNew : 1;
            Word renderFarPlaneIntersectingPolygons : 1;
            Word render1DotPolygonsBehindDisp1DotDepth : 1;
            Word depthTestIsEqual : 1; // 0: less
            Word fogEnable : 1;
            Word alpha : 5;
            Word _unused1 : 3;
            Word polygonID : 6;
            Word _unused2 : 2;
        } state;
        Word raw;
    };

    union TEXIMAGE_PARAM_T{
        struct{
            Word textureVramOffsetDiv8 : 16;
            Word repeatInS : 1;
            Word repeatInT : 1;
            Word flipInS : 1;
            Word flipInT : 1;
            Word textureSSize : 3;
            Word textureTSize : 3;
            Word textureFormat : 3;
            Word colour0transparent : 1;
            Word textureCoordinatesTransformationMode : 2;
        } state;
        Word raw;
    };

    struct Vertex{
        // vec4 worldPos;
        vec4 clipPos;

        int screenX = 0;
        int screenY = 0;
        std::array<int32_t, 6> attributes;

        inline uint8_t r() const { return attributes[0];}
        inline uint8_t g() const { return attributes[1];}
        inline uint8_t b() const { return attributes[2];}
        inline int16_t s() const { return attributes[3];}
        inline int16_t t() const { return attributes[4];}
        inline int32_t z() const { return attributes[5];}
    };

    struct Polygon{
        uint16_t numVertices;
        std::array<uint16_t, 10> vertices; // index in vertex ram
        POLYGON_ATTR_T attr;
        TEXIMAGE_PARAM_T texParam = {.raw = 0};
        Word paletteBaseAddress = 0;
        uint16_t top = 0, bottom = 0; // index in vertices
        int topY, bottomY; // koordinate

        uint16_t curL, curR; // index
        uint16_t nextL, nextR; // index

        bool clockwise = false;

    };
}
