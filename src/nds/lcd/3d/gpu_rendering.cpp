#include "gpu.h"
#include "../2d/ppu_registers.h"
#include "framework/global.h"

using namespace nds;

constexpr inline int64_t applyLinFactor(int64_t amin, int64_t amax, int64_t f){
        return amin + (((amax - amin) * f) >> 16);
};

constexpr inline void interpolateAttribs(const std::array<int32_t, 10> &vCur, const std::array<int32_t, 10> &vNext, const int factor, std::array<int32_t, 10> &result){
    for(int i = 3; i < 9; i++){
        result[i] = applyLinFactor(vCur[i], vNext[i], factor);
    }
}

constexpr inline void interpolateZW(const std::array<int32_t, 10> &vCur, const std::array<int32_t, 10> &vNext, const int factor, std::array<int32_t, 10> &result){
    result[9] = applyLinFactor(vCur[9], vNext[9], factor);
}

void nds::GPU::renderScanline(const int y)
{
    for(int x = 0; x < 256; x++){
        depthBuffer[x + y * 256] = clearDepth * 0x200;
        framebuffer[x + y * 256] = 0;
    }

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
            const int64_t yFactorL = dyL == 0 ? 0 : ((y - curL.screenY) << 16) / dyL; // screen space
            const int64_t divL = (0x10000-yFactorL) * nextL.w() + yFactorL * curL.w();
            const int64_t factorPL = divL > 0 ? (yFactorL * curL.w() << 16) / divL : 0;
            const int64_t yFactorR = dyR == 0 ? 0 : ((y - curR.screenY) << 16) / dyR;
            const int64_t divR = (0x10000-yFactorR) * nextR.w() + yFactorR * curR.w();
            const int64_t factorPR = divR > 0 ? (yFactorR * curR.w() << 16) / divR : 0;
    
            if(curL.screenY != nextL.screenY){
                xL = int64_t(curL.screenX) + int64_t(y - curL.screenY) * int64_t(nextL.screenX - curL.screenX) / int64_t(nextL.screenY - curL.screenY);
            }

            if(curR.screenY != nextR.screenY){
                xR = int64_t(curR.screenX) + int64_t(y - curR.screenY) * int64_t(nextR.screenX - curR.screenX) / int64_t(nextR.screenY - curR.screenY);
            }

            int inc = xR >= xL ? 1 : -1;

            int xLClamp = std::clamp(xL, -1, 257);
            int xRClamp = std::clamp(xR, -1, 257);

            interpolateAttribs(curL.attributes, nextL.attributes, factorPL, render.attributesL);
            interpolateAttribs(curR.attributes, nextR.attributes, factorPR, render.attributesR);

            if(!render.depthBufferingUseW){
                interpolateZW(curL.attributes, nextL.attributes, yFactorL, render.attributesL);
                interpolateZW(curR.attributes, nextR.attributes, yFactorR, render.attributesR);
            }

            for(int x = xLClamp; x != xRClamp + inc; x+= inc){
                if(x >= 0 && x < 256){
                    const int dx = xR - xL;
                    const int64_t xFactor = dx == 0 ? 0 : ((x - xL) << 16) / dx;
                    const int64_t d = (0x10000-xFactor) * render.attributesR[3] + xFactor * render.attributesL[3];
                    const int64_t factorP = d > 0 ? (xFactor * render.attributesL[3] << 16) / d : 0;

                    interpolateAttribs(render.attributesL, render.attributesR, factorP, render.attributesPixel);
                    if(!render.depthBufferingUseW){
                        interpolateZW(render.attributesL, render.attributesR, xFactor, render.attributesPixel);
                    }

                    if(p.attr.state.depthTestIsEqual){
                        int64_t diff = std::abs((render.depthBufferingUseW ? render.attributesPixel[3] : render.attributesPixel[9]) - depthBuffer[x + y * 256]);
                        if(diff > 0x200) continue; // "equal"
                    }
                    else{
                        if(depthBuffer[x + y * 256] < (render.depthBufferingUseW ? render.attributesPixel[3] : render.attributesPixel[9])) continue; // depth test (less)
                    }

                    uint32_t c = getColour(p, render.attributesPixel);

                    if((c >> 24) == 0) continue;

                    framebuffer[x + y * 256] = c;
                    if(render.depthBufferingUseW)
                        depthBuffer[x + y * 256] = render.attributesPixel[3];
                    else
                        depthBuffer[x + y * 256] = render.attributesPixel[9];
                }
            }
        }
    }
}

constexpr static uint32_t mix(const int index0, const int index1, const int weight0, const int weight1, const std::array<uint32_t,4> &cols){
    uint32_t r0 = cols[index0] & 0xFF;
    uint32_t g0 = (cols[index0] >> 8) & 0xFF;
    uint32_t b0 = (cols[index0] >> 16) & 0xFF;
    uint32_t a0 = (cols[index0] >> 24) & 0xFF;

    uint32_t r1 = cols[index1] & 0xFF;
    uint32_t g1 = (cols[index1] >> 8) & 0xFF;
    uint32_t b1 = (cols[index1] >> 16) & 0xFF;
    uint32_t a1 = (cols[index1] >> 24) & 0xFF;

    const int div = weight0 + weight1;

    uint8_t rr = (weight0 * r0 + weight1 * r1) / div;
    uint8_t rg = (weight0 * g0 + weight1 * g1) / div;
    uint8_t rb = (weight0 * b0 + weight1 * b1) / div;
    uint8_t ra = (weight0 * a0 + weight1 * a1) / div;

    return rr | (Word(rg) << 8) | (Word(rb) << 16) | (Word(ra) << 24);
}

constexpr static uint32_t compressedTexel(const std::array<uint32_t, 4> &cols, const uint32_t texel, const uint32_t mode){
    if(mode == 2){
        return cols[texel];
    }
    else if(mode == 0){
        if(texel < 3) return cols[texel];
        return 0; // transparent
    }
    else if(mode == 1){
        if(texel < 2) return cols[texel];
        else if(texel == 2){
            return mix(0,1,1,1, cols);
        }
        return 0; // transparent
    }
    else if(mode == 3){
        if(texel < 2) return cols[texel];
        else if(texel == 2){
            return mix(0,1,5,3, cols);
        }
        else return mix(0,1,3,5, cols);
    }
    return 0;
}

constexpr static uint32_t make32bitColour(const HalfWord colour, const uint8_t alpha){
    const uint8_t r = (colour & 0b11111) << 3;
    const uint8_t g = ((colour >> 5) & 0b11111) << 3;
    const uint8_t b = ((colour >> 10) & 0b11111) << 3;
    return r | (g << 8) | (b << 16) | (alpha << 24);
}

uint32_t nds::GPU::getColour(const Polygon &p, const std::array<int32_t, 10> &attribs)
{
    auto texelS = (attribs[7] >> 4);
    auto texelT = (attribs[8] >> 4);
    const auto sizeS = 8 << p.texParam.state.textureSSize;
    const auto sizeT = 8 << p.texParam.state.textureTSize;
    if(!p.texParam.state.repeatInS) texelS = std::clamp(texelS, 0, sizeS-1);
    if(!p.texParam.state.repeatInT) texelT = std::clamp(texelT, 0, sizeT-1);

    if(p.texParam.state.repeatInS){
        if(p.texParam.state.flipInS && (texelS & sizeS)){
            texelS = sizeS - 1 - (texelS & (sizeS - 1));
        }
        else{
            texelS &= sizeS - 1;
        }
    }

    if(p.texParam.state.repeatInT){
        if(p.texParam.state.flipInT && (texelT & sizeT)){
            texelT = sizeT - 1 - (texelT & (sizeT - 1));
        }
        else{
            texelT &= sizeT - 1;
        }
    }


    if(p.texParam.state.textureFormat == 0){ // keine Textur
        return attribs[4] | (attribs[5] << 8) | (attribs[6] << 16) | (0xFF << 24);
    }
    else if(p.texParam.state.textureFormat == 1){ // 3 bit alpha, 5 bit palette
        const Word texelIndex = texelS + texelT * (8 << p.texParam.state.textureSSize);
        const Word texelBytes = texelIndex;
        const Word texelAddr = (p.texParam.state.textureVramOffsetDiv8 << 3) + texelBytes;
        const Word textureSlot = texelAddr / 0x20000;
        if(textureRearPlane[textureSlot].ptr == nullptr || textureSlot >= 4) return 0;
        const Word textureOffset = texelAddr % 0x20000;
        const Word tx = *(textureRearPlane[textureSlot].ptr + textureOffset);
        const Word paletteIndex = (tx & 0b11111) + p.paletteBaseAddress * 8; // in Halbwort-Einheiten
        const Word preAlpha = (tx >> 5);
        const Word alpha = ((preAlpha * 4) + (preAlpha / 2)) << 3;
        // if(paletteIndex == 0 && p.texParam.state.colour0transparent) return 0;
        const Word paletteSlot = paletteIndex / 0x2000; // 16KiB (0x4000, 0x2000 Halbwörter)
        if(texturePalette[paletteSlot].ptr == nullptr || paletteSlot >= 6) return 0;
        const uint16_t colour = *(((uint16_t*)(texturePalette[paletteSlot].ptr) + (paletteIndex % 0x2000)));
        return make32bitColour(colour, alpha);
    }
    else if(p.texParam.state.textureFormat == 2){
        const Word texelIndex = texelS + texelT * (8 << p.texParam.state.textureSSize);
        const Word texelBytes = texelIndex / 4; // 2bbp * 4 = 1 byte
        const Word texelBits = texelIndex % 4;
        const Word texelAddr = (p.texParam.state.textureVramOffsetDiv8 << 3) + texelBytes;
        const Word textureSlot = texelAddr / 0x20000;
        if(textureRearPlane[textureSlot].ptr == nullptr || textureSlot >= 4) return 0;
        const Word textureOffset = texelAddr % 0x20000;
        const Word paletteIndex = (*(textureRearPlane[textureSlot].ptr + textureOffset) >> (texelBits * 2)) & 0b11;
        const Word palWithBase = paletteIndex + p.paletteBaseAddress * 4;
        if(paletteIndex == 0 && p.texParam.state.colour0transparent) return 0;
        const Word paletteSlot = palWithBase / 0x2000;
        if(texturePalette[paletteSlot].ptr == nullptr || paletteSlot >= 6) return 0;
        const uint16_t colour = *(((uint16_t*)(texturePalette[paletteSlot].ptr) + (palWithBase % 0x2000)));
        return make32bitColour(colour, 0xFF);
    }
    else if(p.texParam.state.textureFormat == 3){
        const Word texelIndex = texelS + texelT * (8 << p.texParam.state.textureSSize);
        const Word texelBytes = texelIndex / 2; // 4bbp * 2 = 1 byte
        const Word texelBits = texelIndex % 2;
        const Word texelAddr = (p.texParam.state.textureVramOffsetDiv8 << 3) + texelBytes;
        const Word textureSlot = texelAddr / 0x20000;
        if(textureRearPlane[textureSlot].ptr == nullptr || textureSlot >= 4) return 0;
        const Word textureOffset = texelAddr % 0x20000;
        const Word paletteIndex = (*(textureRearPlane[textureSlot].ptr + textureOffset) >> (texelBits * 4)) & 0xF;
        const Word palWithBase = paletteIndex + p.paletteBaseAddress * 8;
        if(paletteIndex == 0 && p.texParam.state.colour0transparent) return 0;
        const Word paletteSlot = palWithBase / 0x2000;
        if(texturePalette[paletteSlot].ptr == nullptr || paletteSlot >= 6) return 0;
        const uint16_t colour = *(((uint16_t*)(texturePalette[paletteSlot].ptr) + (palWithBase % 0x2000)));
        return make32bitColour(colour, 0xFF);
    }
    else if(p.texParam.state.textureFormat == 4){
        const Word texelIndex = texelS + texelT * (8 << p.texParam.state.textureSSize);
        const Word texelBytes = texelIndex;
        const Word texelAddr = (p.texParam.state.textureVramOffsetDiv8 << 3) + texelBytes;
        const Word textureSlot = texelAddr / 0x20000;
        if(textureRearPlane[textureSlot].ptr == nullptr || textureSlot >= 4) return 0;
        const Word textureOffset = texelAddr % 0x20000;
        const Word paletteIndex = *(textureRearPlane[textureSlot].ptr + textureOffset);
        const Word palWithBase = paletteIndex + p.paletteBaseAddress * 8;
        if(paletteIndex == 0 && p.texParam.state.colour0transparent) return 0;
        const Word paletteSlot = palWithBase / 0x2000;
        if(texturePalette[paletteSlot].ptr == nullptr || paletteSlot >= 6) return 0;
        const uint16_t colour = *(((uint16_t*)(texturePalette[paletteSlot].ptr) + (palWithBase % 0x2000)));
        return make32bitColour(colour, 0xFF);
    }
    else if(p.texParam.state.textureFormat == 5){
        const Word block = (texelS / 4) + (texelT / 4) * ((8 << p.texParam.state.textureSSize) / 4); // 4x4 block
        const Word blockAddr = (p.texParam.state.textureVramOffsetDiv8 << 3) + block * 4;
        const Word texelAddr = blockAddr + (texelT & 0b11);
        const Word textureSlot = texelAddr / 0x20000;
        if(textureRearPlane[textureSlot].ptr == nullptr || (textureSlot != 0 && textureSlot != 2)) return 0;
        const Word textureOffset = texelAddr % 0x20000;
        const Word texelVal = (*(textureRearPlane[textureSlot].ptr + textureOffset) >> (2 * (texelS & 0b11))) & 0b11;
        
        Word palAddr = (blockAddr % 0x20000) / 2;
        if(textureSlot == 2) palAddr += 0x10000; // Bei Slot 2 ist es im hinteren Teil
        if(textureRearPlane[1].ptr == nullptr) return 0;
        const HalfWord palIndexData = *(HalfWord*)(textureRearPlane[1].ptr + palAddr);
        const Word mode = palIndexData >> 14;
        const Word palOffset = ((palIndexData << 2) & 0xFFFF) + p.paletteBaseAddress * 16;

        const Word paletteSlot = palOffset / 0x4000;
        if(texturePalette[paletteSlot].ptr == nullptr || paletteSlot >= 6) return 0;

        const HalfWord* paletteStart = ((uint16_t*)((texturePalette[paletteSlot].ptr) + (palOffset % 0x4000))); // Achtung, palOffset ist in BYTES

        const std::array<uint32_t, 4> colours{make32bitColour(*(paletteStart), 0xFF), make32bitColour(*(paletteStart + 1), 0xFF), make32bitColour(*(paletteStart + 2), 0xFF), make32bitColour(*(paletteStart + 3), 0xFF)};
        return compressedTexel(colours, texelVal, mode);
    }
    else if(p.texParam.state.textureFormat == 6){
        const Word texelIndex = texelS + texelT * (8 << p.texParam.state.textureSSize);
        const Word texelBytes = texelIndex;
        const Word texelAddr = (p.texParam.state.textureVramOffsetDiv8 << 3) + texelBytes;
        const Word textureSlot = texelAddr / 0x20000;
        if(textureRearPlane[textureSlot].ptr == nullptr || textureSlot >= 4) return 0;
        const Word textureOffset = texelAddr % 0x20000;
        const Word tx = *(textureRearPlane[textureSlot].ptr + textureOffset);
        const Word paletteIndex = (tx & 0b111) + p.paletteBaseAddress * 8;
        const Word alpha = (tx >> 3) << 3;
        // if(paletteIndex == 0 && p.texParam.state.colour0transparent) return 0;
        const Word paletteSlot = paletteIndex / 0x2000;
        if(texturePalette[paletteSlot].ptr == nullptr || paletteSlot >= 6) return 0;
        const uint16_t colour = *(((uint16_t*)(texturePalette[paletteSlot].ptr) + (paletteIndex % 0x2000)));
        return make32bitColour(colour, alpha);
    }
    else if(p.texParam.state.textureFormat == 7){
        const Word texelIndex = texelS + texelT * (8 << p.texParam.state.textureSSize);
        const Word texelAddr = (p.texParam.state.textureVramOffsetDiv8 << 3) + 2 * texelIndex;
        const Word textureSlot = texelAddr / 0x20000;
        if(textureRearPlane[textureSlot].ptr == nullptr || textureSlot >= 4) return 0;
        const Word textureOffset = texelAddr % 0x20000;
        const HalfWord colour = *((uint16_t*)(textureRearPlane[textureSlot].ptr + textureOffset));
        return make32bitColour(colour, 0xFF * (colour >> 15));
    }
    return 0;
}

uint32_t nds::GPU::fetchTexel(const Polygon &p, int32_t s, int32_t t)
{
    return 0;
}
