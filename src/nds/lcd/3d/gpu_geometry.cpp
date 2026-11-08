#include "gpu.h"
#include "../../bus.h"
#include "framework/stringlib.h"

using namespace nds;

void nds::GPU::clockGeometry()
{
    if(commandFifo.size() > 0 && !haltGeometry){
        // ausführen
        const auto f = cmdToFunction[commandFifo.front().cmd];
        const auto sz = (unsigned int)cmdToNumParams[commandFifo.front().cmd];
        (this->*f)(commandFifo.front());
        if(commandFifoSize < std::max(sz, 1u)){
            std::cout << "WRONG COMMAND FIFO HANDLING, PARAMETER LENGTH EXCEEDS FIFO SIZE\n";
            std::terminate();
        }
        commandFifoSize -= std::max(sz, 1u);
        commandFifo.pop();
    }

    updateIF();
}

void nds::GPU::transformVertex(Vertex &v)
{
    v.clipPos = v.clipPos * clipMtxResult;
    if(v.clipPos.w > 0){
        v.screenX = (v.clipPos.x + v.clipPos.w) * (viewport.state.x2 - viewport.state.x1) / (2*v.clipPos.w) + viewport.state.x1;
        v.screenY = (v.clipPos.y + v.clipPos.w) * (viewport.state.y2 - viewport.state.y1) / (2*v.clipPos.w) + viewport.state.y1;
    }
    else{
        v.screenX = -1;
        v.screenY = -1;
    }
}

void nds::GPU::swapBuffers() // Das ist die tatsächliche Ausführung nach VBlank
{
    vertexPolyRam.swap(vertexPolyRamRender);
    vertexPolyRam = {};
    // std::fill(vertexPolyRam.begin(), vertexPolyRam.end(), Vertex{}); // leeren und an geometrie engine geben
    haltGeometry = false;
}

size_t &nds::GPU::getSP()
{
    switch(geometry.mtxMode){
        case 0: return matrixStack.projectionSP;
        case 1: return matrixStack.coordinateSP;
        case 2: return matrixStack.coordinateSP;
        case 3: return matrixStack.textureSP;
        default: return matrixStack.projectionSP;
    }
}

std::array<mat4x4, 32> &nds::GPU::getStack()
{
    switch(geometry.mtxMode){
        case 0: return matrixStack.projection;
        case 1: return matrixStack.coordinate;
        case 2: return matrixStack.coordinate;
        case 3: return matrixStack.texture;
        default: return matrixStack.projection;
    }
}

void nds::GPU::recalculateClipMtx()
{
    clipMtxResult = matrixStack.currentCoordinate * matrixStack.currentProjection;
}

void nds::GPU::NOP(const FIFOCommand &cmd)
{
    if(cmd.cmd != 0){
        std::cout << "Unknown FIFO CMD: " << getHex0x(cmd.cmd, 2) << "\n";
    }
}

void nds::GPU::MTX_PUSH(const FIFOCommand &cmd)
{
    auto &sp = getSP();
    auto &stack = getStack();
    if(sp > 30) GXSTAT.state.matrixStackOverUnderflow = 1;
    if(geometry.mtxMode == 1 || geometry.mtxMode == 2){
        stack[sp & 0x1F] = *matrixStack.current;
        matrixStack.directional[sp & 0x1F] = matrixStack.currentDirection;
    }
    else{
        stack[0] = *matrixStack.current;
        if(sp>=1) GXSTAT.state.matrixStackOverUnderflow = 1;
    }
    sp++;
    sp &= 0x3F;
    if(geometry.mtxMode == 0 || geometry.mtxMode == 3){
        sp &= 1;
    }
}

void nds::GPU::MTX_POP(const FIFOCommand &cmd)
{
    Word nr = cmd.params[0] & 0b111111;
    int32_t n = sign_extend_n_32(nr, 6);
    auto &sp = getSP();
    if(geometry.mtxMode == 0 || geometry.mtxMode == 3){
        n = 1;
        if(sp == 0) GXSTAT.state.matrixStackOverUnderflow = 1;
    }

    sp -= n;
    sp &= 0x3F;
    if(sp > 30) GXSTAT.state.matrixStackOverUnderflow = 1;

    auto &stack = getStack();
    if(geometry.mtxMode == 1 || geometry.mtxMode == 2){
        *matrixStack.current = stack[sp & 0x1F];
        matrixStack.currentDirection = matrixStack.directional[sp & 0x1F];
    }
    else{
        sp &= 1;
        *matrixStack.current = stack[0];
    }

    if(geometry.mtxMode <= 2) recalculateClipMtx();
}

void nds::GPU::MTX_STORE(const FIFOCommand &cmd)
{
    auto &stack = getStack();
    Word n = cmd.params[0] & 0b11111;
    if(geometry.mtxMode == 0 || geometry.mtxMode == 3) n = 0;
    stack[n] = *matrixStack.current;
    if(geometry.mtxMode == 1 || geometry.mtxMode == 2) matrixStack.directional[n] = matrixStack.currentDirection;
    if(n > 30) GXSTAT.state.matrixStackOverUnderflow = 1;
}

void nds::GPU::MTX_RESTORE(const FIFOCommand &cmd)
{
    auto &stack = getStack();
    Word n = cmd.params[0] & 0b11111;
    if(geometry.mtxMode == 0 || geometry.mtxMode == 3) n = 0;
    *matrixStack.current = stack[n];
    if(geometry.mtxMode == 1 || geometry.mtxMode == 2) matrixStack.currentDirection = matrixStack.directional[n];
    if(n > 30) GXSTAT.state.matrixStackOverUnderflow = 1;

    if(geometry.mtxMode <= 2) recalculateClipMtx();
}

void nds::GPU::MTX_IDENTITY(const FIFOCommand &cmd)
{
    *matrixStack.current = mat4x4::identity();
    if(geometry.mtxMode == 2) matrixStack.currentDirection = mat4x4::identity(); // current ist Position / Coordinate

    if(geometry.mtxMode <= 2) recalculateClipMtx();
}

void nds::GPU::MTX_LOAD_4x4(const FIFOCommand &cmd)
{
    for(int i = 0; i < 16; i++){
        matrixStack.current->data[i] = cmd.params[i];
        if(geometry.mtxMode == 2) matrixStack.currentDirection.data[i] = cmd.params[i];
    }

    if(geometry.mtxMode <= 2) recalculateClipMtx();
}

void nds::GPU::MTX_LOAD_4x3(const FIFOCommand &cmd)
{
    int it = 0;
    for(int i = 0; i < 4; i++){
        for(int j = 0; j < 3; j++){
            (*matrixStack.current)(i,j) = cmd.params[it];
            if(geometry.mtxMode == 2) matrixStack.currentDirection(i,j) = cmd.params[it];
            it++;
        }
    }

    for(int i = 0; i < 3; i++){
        (*matrixStack.current)(i,3) = 0;
        if(geometry.mtxMode == 2) matrixStack.currentDirection(i,3) = 0;
    }

    (*matrixStack.current)(3,3) = 0x1000;
    if(geometry.mtxMode == 2) matrixStack.currentDirection(3,3) = 0x1000;

    if(geometry.mtxMode <= 2) recalculateClipMtx();
}

void nds::GPU::MTX_MULT_4x4(const FIFOCommand &cmd)
{
    mat4x4 m;
    for(int i = 0; i < 16; i++){
        m.data[i] = cmd.params[i];
    }

    *matrixStack.current = m * *matrixStack.current;
    if(geometry.mtxMode == 2) matrixStack.currentDirection = m * matrixStack.currentDirection;

    if(geometry.mtxMode <= 2) recalculateClipMtx();
}

void nds::GPU::MTX_MULT_4x3(const FIFOCommand &cmd)
{
    mat4x4 m = mat4x4::identity();
    int it = 0;
    for(int i = 0; i < 4; i++){
        for(int j = 0; j < 3; j++){
            m(i,j) = cmd.params[it];
            it++;
        }
    }

    *matrixStack.current = m * *matrixStack.current;
    if(geometry.mtxMode == 2) matrixStack.currentDirection = m * matrixStack.currentDirection;

    if(geometry.mtxMode <= 2) recalculateClipMtx();
}

void nds::GPU::MTX_MULT_3x3(const FIFOCommand &cmd)
{
    mat4x4 m = mat4x4::identity();
    int it = 0;
    for(int i = 0; i < 3; i++){
        for(int j = 0; j < 3; j++){
            m(i,j) = cmd.params[it];
            it++;
        }
    }

    *matrixStack.current = m * *matrixStack.current;
    if(geometry.mtxMode == 2) matrixStack.currentDirection = m * matrixStack.currentDirection;

    if(geometry.mtxMode <= 2) recalculateClipMtx();
}

void nds::GPU::MTX_SCALE(const FIFOCommand &cmd)
{
    mat4x4 scale = mat4x4::identity();
    for(int i = 0; i < 3; i++){
        scale(i,i) = cmd.params[i];
    }

    *matrixStack.current = scale * *matrixStack.current;
    // Direction Matrix wird in Mode 2 nicht verändert

    if(geometry.mtxMode <= 2) recalculateClipMtx();
}

void nds::GPU::MTX_TRANS(const FIFOCommand &cmd)
{
    mat4x4 translate = mat4x4::identity();
    for(int i = 0; i < 3; i++){
        translate(3,i) = cmd.params[i];
    }

    *matrixStack.current = translate * *matrixStack.current;
    if(geometry.mtxMode == 2) matrixStack.currentDirection = translate * matrixStack.currentDirection;

    if(geometry.mtxMode <= 2) recalculateClipMtx();
}

void nds::GPU::COLOR(const FIFOCommand &cmd)
{
}

void nds::GPU::NORMAL(const FIFOCommand &cmd)
{
}

void nds::GPU::TEXCOORD(const FIFOCommand &cmd)
{
}

void nds::GPU::VTX_16(const FIFOCommand &cmd)
{
    const int32_t x = sign_extend_n_32(cmd.params[0] & 0xFFFF, 16);
    const int32_t y = sign_extend_n_32((cmd.params[0] >> 16) & 0xFFFF, 16);
    const int32_t z = sign_extend_n_32(cmd.params[1] & 0xFFFF, 16);
    geometry.p = vec4(x,y,z);
    Vertex v{.clipPos = geometry.p};
    transformVertex(v);
    vertexPolyRam.push_back(v);
}

void nds::GPU::VTX_10(const FIFOCommand &cmd)
{
    const int32_t x = sign_extend_n_32(cmd.params[0] & 0x3FF, 10);
    const int32_t y = sign_extend_n_32((cmd.params[0] >> 10) & 0x3FF, 10);
    const int32_t z = sign_extend_n_32((cmd.params[0] >> 20) & 0x3FF, 10);
    geometry.p = vec4(x << 6,y << 6,z << 6); // Fractional auf 12 bit?
    Vertex v{.clipPos = geometry.p};
    transformVertex(v);
    vertexPolyRam.push_back(v);
}

void nds::GPU::VTX_XY(const FIFOCommand &cmd)
{
    const int32_t x = sign_extend_n_32(cmd.params[0] & 0xFFFF, 16);
    const int32_t y = sign_extend_n_32((cmd.params[0] >> 16) & 0xFFFF, 16);
    const int32_t z = geometry.p.z;
    geometry.p = vec4(x,y,z);
    Vertex v{.clipPos = geometry.p};
    transformVertex(v);
    vertexPolyRam.push_back(v);
}

void nds::GPU::VTX_XZ(const FIFOCommand &cmd)
{
    const int32_t x = sign_extend_n_32(cmd.params[0] & 0xFFFF, 16);
    const int32_t z = sign_extend_n_32((cmd.params[0] >> 16) & 0xFFFF, 16);
    const int32_t y = geometry.p.y;
    geometry.p = vec4(x,y,z);
    Vertex v{.clipPos = geometry.p};
    transformVertex(v);
    vertexPolyRam.push_back(v);
}

void nds::GPU::VTX_YZ(const FIFOCommand &cmd)
{
    const int32_t y = sign_extend_n_32(cmd.params[0] & 0xFFFF, 16);
    const int32_t z = sign_extend_n_32((cmd.params[0] >> 16) & 0xFFFF, 16);
    const int32_t x = geometry.p.x;
    geometry.p = vec4(x,y,z);
    Vertex v{.clipPos = geometry.p};
    transformVertex(v);
    vertexPolyRam.push_back(v);
}

void nds::GPU::VTX_DIFF(const FIFOCommand &cmd)
{
    const int32_t x = sign_extend_n_32(cmd.params[0] & 0x3FF, 10) >> 3;
    const int32_t y = sign_extend_n_32((cmd.params[0] >> 10) & 0x3FF, 10) >> 3;
    const int32_t z = sign_extend_n_32((cmd.params[0] >> 20) & 0x3FF, 10) >> 3;

    geometry.p = vec4(geometry.p.x + x,geometry.p.y + y,geometry.p.z + z);
    Vertex v{.clipPos = geometry.p};
    transformVertex(v);
    vertexPolyRam.push_back(v);
}

void nds::GPU::POLYGON_ATTR(const FIFOCommand &cmd)
{
    geometry.nextPolygonAttr = POLYGON_ATTR_T{.raw = cmd.params[0]};
}

void nds::GPU::TEXIMAGE_PARAM(const FIFOCommand &cmd)
{
}

void nds::GPU::PLTT_BASE(const FIFOCommand &cmd)
{
}

void nds::GPU::DIF_AMB(const FIFOCommand &cmd)
{
}

void nds::GPU::SPE_EMI(const FIFOCommand &cmd)
{
}

void nds::GPU::LIGHT_VECTOR(const FIFOCommand &cmd)
{
}

void nds::GPU::LIGHT_COLOR(const FIFOCommand &cmd)
{
}

void nds::GPU::SHININESS(const FIFOCommand &cmd)
{
}

void nds::GPU::BEGIN_VTXS(const FIFOCommand &cmd)
{
    geometry.polygonAttr = geometry.nextPolygonAttr;
    geometry.primitiveType = cmd.params[0] & 3;
}

void nds::GPU::END_VTXS(const FIFOCommand &cmd)
{
}

void nds::GPU::SWAP_BUFFERS(const FIFOCommand &cmd)
{
    // Bei nächstem Vblank
    bus->lcd->swap3DBuffers = true;
    haltGeometry = true;
    geometry.manualTranslucentPolygonYSorting = (cmd.params[0] & 1u) > 0;
    geometry.depthBufferingUseW = (cmd.params[0] & 2u) > 0;
}

void nds::GPU::VIEWPORT(const FIFOCommand &cmd)
{
    viewport.raw = cmd.params[0];
}

void nds::GPU::BOX_TEST(const FIFOCommand &cmd)
{
}

void nds::GPU::POS_TEST(const FIFOCommand &cmd)
{
}

void nds::GPU::VEC_TEST(const FIFOCommand &cmd)
{
}

void nds::GPU::MTX_MODE(const FIFOCommand &cmd)
{
    geometry.mtxMode = cmd.params[0] & 0b11;
    if(geometry.mtxMode == 0) matrixStack.current = &matrixStack.currentProjection;
    else if(geometry.mtxMode == 1) matrixStack.current = &matrixStack.currentCoordinate;
    else if(geometry.mtxMode == 2) matrixStack.current = &matrixStack.currentCoordinate;
    else if(geometry.mtxMode == 3) matrixStack.current = &matrixStack.currentTexture;
}
