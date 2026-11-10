#include "gpu.h"
#include "../2d/ppu_registers.h"
#include "framework/global.h"

using namespace nds;

PIXEL_T nds::GPU::renderPixelTest(int x, int y)
{
    for(const auto &e : vertexPolyRamRender){
        if(std::holds_alternative<Triangle>(e)){
            for(const auto &v : std::get<Triangle>(e).vertices){
                if(v.screenX == x && v.screenY == y && v.clipPos.z > 0 && v.clipPos.w > 0) return PIXEL_T{.pixel = 1, .red = 255, .green = 255, .blue = 255};
            }
        }
        else{
            for(const auto &v : std::get<Quad>(e).vertices){
                if(v.screenX == x && v.screenY == y && v.clipPos.z > 0 && v.clipPos.w > 0) return PIXEL_T{.pixel = 1, .red = 255, .green = 255, .blue = 255};
            }
        }
    }
    return PIXEL_T{.pixel = 0};
}

void nds::GPU::renderScanline(const int ry)
{
    const int y = 191 - ry;
    for(int x = 0; x < 256; x++){
        framebuffer[x + ry * 256] = 0;
        for(const auto &e : vertexPolyRamRender){
            if(std::holds_alternative<Triangle>(e)){
                for(const auto &v : std::get<Triangle>(e).vertices){
                    if(v.screenX == x && v.screenY == y && v.clipPos.z > 0 && v.clipPos.w > 0) framebuffer[x + ry * 256] = 0xFFFFFFFF;
                }
            }
            else{
                for(const auto &v : std::get<Quad>(e).vertices){
                    if(v.screenX == x && v.screenY == y && v.clipPos.z > 0 && v.clipPos.w > 0) framebuffer[x + ry * 256] = 0xFFFFFFFF;
                }
            }
        }
    }
}
