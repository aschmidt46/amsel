#include "gpu.h"
#include "../2d/ppu_registers.h"
#include "framework/global.h"

using namespace nds;

constexpr inline int64_t applyLinFactor(int64_t amin, int64_t amax, int64_t f){
        return amin + (((amax - amin) * f) >> 16);
};

constexpr inline void interpolateAttribs(const std::array<int32_t, 6> &vCur, const std::array<int32_t, 6> &vNext, const int factor, std::array<int32_t, 6> &result){
    for(int i = 0; i < 6; i++){
        result[i] = applyLinFactor(vCur[i], vNext[i], factor);
    }
}

void nds::GPU::renderScanline(const int y)
{
    for(int x = 0; x < 256; x++) framebuffer[x + y * 256] = 0;

    for(auto &p : polyRamRender){
        if(y >= p.topY && y < p.bottomY){
            maybeAdvanceEdgeWalk(vertexRamRender, p, y);
            const Vertex& curL = vertexRamRender[p.vertices[p.curL]];
            const Vertex& nextL = vertexRamRender[p.vertices[p.nextL]];
            const Vertex& curR = vertexRamRender[p.vertices[p.curR]];
            const Vertex& nextR = vertexRamRender[p.vertices[p.nextR]];
            int xL = curL.screenX;
            int xR = curR.screenX;

            const int dyL = nextL.screenY - curL.screenY;
            const int dyR = nextR.screenY - curR.screenY;
            const int yFactorL = dyL == 0 ? 0 : ((y - curL.screenY) << 16) / dyL;
            const int yFactorR = dyR == 0 ? 0 : ((y - curR.screenY) << 16) / dyR;
    
            if(curL.screenY != nextL.screenY){
                xL = int64_t(curL.screenX) + int64_t(y - curL.screenY) * int64_t(nextL.screenX - curL.screenX) / int64_t(nextL.screenY - curL.screenY);
            }

            if(curR.screenY != nextR.screenY){
                xR = int64_t(curR.screenX) + int64_t(y - curR.screenY) * int64_t(nextR.screenX - curR.screenX) / int64_t(nextR.screenY - curR.screenY);
            }
            if(xR < xL){
                const auto tmp = xR;
                xR = xL;
                xL = tmp;
            }

            xL = std::clamp(xL, -1, 257);
            xR = std::clamp(xR, -1, 257);

            interpolateAttribs(curL.attributes, nextL.attributes, yFactorL, render.attributesL);
            interpolateAttribs(curR.attributes, nextR.attributes, yFactorR, render.attributesR);

            for(int x = xL; x <= xR; x++){
                if(x >= 0 && x < 256){
                    const int dx = xR - xL;
                    const int xFactor = dx == 0 ? 0 : ((x - xL) << 16) / dx;

                    interpolateAttribs(render.attributesL, render.attributesR, xFactor, render.attributesPixel);

                    uint32_t c = getColour(p, render.attributesPixel);

                    framebuffer[x + y * 256] = c;
                }
            }
        }
    }
}

uint32_t nds::GPU::getColour(const Polygon &p, const std::array<int32_t, 6> &attribs)
{
    if(p.texParam.state.textureFormat == 0){ // keine Textur
        return attribs[0] | (attribs[1] << 8) | (attribs[2] << 16) | (0xFF << 24);
    }
    else if(p.texParam.state.textureFormat == 2){
        const auto texelS = (attribs[3] >> 4) & ((8 << p.texParam.state.textureSSize) - 1);
        const auto texelT = (attribs[4] >> 4) & ((8 << p.texParam.state.textureTSize) - 1);
        const Word texelIndex = texelS + texelT * (8 << p.texParam.state.textureSSize);
        const Word texelBytes = texelIndex / 4; // 2bbp * 4 = 1 byte
        const Word texelBits = texelIndex % 4;
        const Word texelAddr = (p.texParam.state.textureVramOffsetDiv8 << 3) + texelBytes;
        const Word textureSlot = texelAddr / 0x20000;
        if(textureRearPlane[textureSlot].ptr == nullptr || textureSlot >= 4) return 0;
        const Word textureOffset = texelAddr % 0x20000;
        const Word paletteIndex = (*(textureRearPlane[textureSlot].ptr + textureOffset) >> (texelBits * 2)) & 0b11;
        if(paletteIndex == 0 && p.texParam.state.colour0transparent) return 0;
        const Word paletteSlot = paletteIndex / 0x4000;
        if(texturePalette[paletteSlot].ptr == nullptr || paletteSlot >= 6) return 0;
        const uint16_t colour = *(((uint16_t*)(texturePalette[paletteSlot].ptr + p.paletteBaseAddress * 8) + (paletteIndex % 0x4000)));
        const uint8_t r = (colour & 0b11111) << 3;
        const uint8_t g = ((colour >> 5) & 0b11111) << 3;
        const uint8_t b = ((colour >> 10) & 0b11111) << 3;
        return r | (g << 8) | (b << 16) | (0xFF << 24);
    }
    else if(p.texParam.state.textureFormat == 3){
        const auto texelS = (attribs[3] >> 4) & ((8 << p.texParam.state.textureSSize) - 1);
        const auto texelT = (attribs[4] >> 4) & ((8 << p.texParam.state.textureTSize) - 1);
        const Word texelIndex = texelS + texelT * (8 << p.texParam.state.textureSSize);
        const Word texelBytes = texelIndex / 2; // 4bbp * 2 = 1 byte
        const Word texelBits = texelIndex % 2;
        const Word texelAddr = (p.texParam.state.textureVramOffsetDiv8 << 3) + texelBytes;
        const Word textureSlot = texelAddr / 0x20000;
        if(textureRearPlane[textureSlot].ptr == nullptr || textureSlot >= 4) return 0;
        const Word textureOffset = texelAddr % 0x20000;
        const Word paletteIndex = (*(textureRearPlane[textureSlot].ptr + textureOffset) >> (texelBits * 4)) & 0xF;
        if(paletteIndex == 0 && p.texParam.state.colour0transparent) return 0;
        const Word paletteSlot = paletteIndex / 0x4000;
        if(texturePalette[paletteSlot].ptr == nullptr || paletteSlot >= 6) return 0;
        const uint16_t colour = *(((uint16_t*)(texturePalette[paletteSlot].ptr + p.paletteBaseAddress * 16) + (paletteIndex % 0x4000)));
        const uint8_t r = (colour & 0b11111) << 3;
        const uint8_t g = ((colour >> 5) & 0b11111) << 3;
        const uint8_t b = ((colour >> 10) & 0b11111) << 3;
        return r | (g << 8) | (b << 16) | (0xFF << 24);
    }
    else if(p.texParam.state.textureFormat == 4){
        const auto texelS = (attribs[3] >> 4) & ((8 << p.texParam.state.textureSSize) - 1);
        const auto texelT = (attribs[4] >> 4) & ((8 << p.texParam.state.textureTSize) - 1);
        const Word texelIndex = texelS + texelT * (8 << p.texParam.state.textureSSize);
        const Word texelBytes = texelIndex;
        const Word texelAddr = (p.texParam.state.textureVramOffsetDiv8 << 3) + texelBytes;
        const Word textureSlot = texelAddr / 0x20000;
        if(textureRearPlane[textureSlot].ptr == nullptr || textureSlot >= 4) return 0;
        const Word textureOffset = texelAddr % 0x20000;
        const Word paletteIndex = *(textureRearPlane[textureSlot].ptr + textureOffset);
        if(paletteIndex == 0 && p.texParam.state.colour0transparent) return 0;
        const Word paletteSlot = paletteIndex / 0x4000;
        if(texturePalette[paletteSlot].ptr == nullptr || paletteSlot >= 6) return 0;
        const uint16_t colour = *(((uint16_t*)(texturePalette[paletteSlot].ptr + p.paletteBaseAddress * 16) + (paletteIndex % 0x4000)));
        const uint8_t r = (colour & 0b11111) << 3;
        const uint8_t g = ((colour >> 5) & 0b11111) << 3;
        const uint8_t b = ((colour >> 10) & 0b11111) << 3;
        return r | (g << 8) | (b << 16) | (0xFF << 24);
    }
    else if(p.texParam.state.textureFormat == 7){
        const auto texelS = (attribs[3] >> 4) & ((8 << p.texParam.state.textureSSize) - 1);
        const auto texelT = (attribs[4] >> 4) & ((8 << p.texParam.state.textureTSize) - 1);
        const Word texelIndex = texelS + texelT * (8 << p.texParam.state.textureSSize);
        const Word texelBytes = texelIndex;
        const Word texelAddr = (p.texParam.state.textureVramOffsetDiv8 << 3) + texelBytes;
        const Word textureSlot = texelAddr / 0x20000;
        if(textureRearPlane[textureSlot].ptr == nullptr || textureSlot >= 4) return 0;
        const Word textureOffset = texelAddr % 0x20000;
        const HalfWord colour = *(((uint16_t*)textureRearPlane[textureSlot].ptr) + textureOffset);
        const uint8_t r = (colour & 0b11111) << 3;
        const uint8_t g = ((colour >> 5) & 0b11111) << 3;
        const uint8_t b = ((colour >> 10) & 0b11111) << 3;
        return r | (g << 8) | (b << 16) | (0xFF << 24);
    }
    return 0;
}
