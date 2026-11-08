#include "gpu.h"
#include "../2d/ppu_registers.h"

using namespace nds;

PIXEL_T nds::GPU::renderPixelTest(int x, int y)
{
    for(const auto &e : vertexPolyRamRender){
        if(e.screenX == x && e.screenY == y && e.clipPos.z > 0 && e.clipPos.w > 0) return PIXEL_T{.pixel = 1, .red = 255, .green = 255, .blue = 255};
    }
    return PIXEL_T{.pixel = 0};
}
