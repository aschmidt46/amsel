#pragma once
#include "../../bus_types.h"
#include <array>

namespace nds{
    struct GPUSlotAllocation{
        Byte* ptr = nullptr;
        int allocatedTo = 0;
    };
    class GPU{
        // Nur Stub für Texture / Palette Slots in VRAM

        public:
        std::array<GPUSlotAllocation, 4> textureRearPlane = {{nullptr, 0}}; // Jeweils 128KB
        std::array<GPUSlotAllocation, 6> texturePalette = {{nullptr, 0}}; // Jeweils 16KB
    };
}
