#include "gpu.h"
#include "../../bus.h"
#include "framework/stringlib.h"
#include "framework/global.h"

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
    vec4 v4(v.x(), v.y(), v.z(), v.w());
    v4 = v4 * clipMtxResult;
    v.x() = v4.x;
    v.y() = v4.y;
    v.z() = v4.z;
    v.w() = v4.w;
    v.r() = (geometry.colour.state.red * 2 + (geometry.colour.state.red + 31) / 32) << 2;
    v.g() = (geometry.colour.state.green * 2 + (geometry.colour.state.green + 31) / 32) << 2;
    v.b() = (geometry.colour.state.blue * 2 + (geometry.colour.state.blue + 31) / 32) << 2;
    v.s() = geometry.nextS;
    v.t() = geometry.nextT;
    v.zw() = v.w() > 0 ? v.z() : -1;
}

void nds::GPU::vertexViewportTransform(Vertex &v)
{
    if(v.w() > 0){
        int64_t sx = (int64_t(v.x()) + v.w()) * (int64_t(viewport.state.x2) - viewport.state.x1) / (int64_t(2)*v.w()) + int64_t(viewport.state.x1);
        int64_t sy = (int64_t(-v.y()) + v.w()) * (int64_t(viewport.state.y2) - viewport.state.y1) / (int64_t(2)*v.w()) + int64_t(viewport.state.y1);
        v.screenX = sx;
        v.screenY = sy;
    }
    else{
        v.screenX = -1;
        v.screenY = -1;
    }
}

void nds::GPU::swapBuffers() // Das ist die tatsächliche Ausführung nach VBlank
{
    vertexRam.swap(vertexRamRender);
    polyRam.swap(polyRamRender);
    for(auto &i : currentPolyIndices){
        i = 0xFFFF;
    }
    vertexRam.clear();
    polyRam.clear();
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

void nds::GPU::addVertex(const Vertex &v)
{
    switch(geometry.primitiveType){
        case 0:{ // dreiecke
            currentPolyVertices[vertexIndex] = v;
            currentPolyIndices[vertexIndex] = 0xFFFF; // aka noch nicht in vertex ram
            vertexIndex++;
            if(vertexIndex > 2){
                if(polygonInViewport(currentPolygon))
                    pushPolygon();
                vertexIndex = 0;
            }
            break;
        }
        case 1:{ // vierecke
            currentPolyVertices[vertexIndex] = v;
            currentPolyIndices[vertexIndex] = 0xFFFF;
            vertexIndex++;
            if(vertexIndex > 3){
                if(polygonInViewport(currentPolygon))
                    pushPolygon();
                vertexIndex = 0;
            }
            break;
        }
        case 2:{ // dreieck-strip
            currentPolyVertices[vertexIndex] = v;
            currentPolyIndices[vertexIndex] = 0xFFFF;
            vertexIndex++;
            if(vertexIndex > 2){
                if(polygonInViewport(currentPolygon))
                    pushPolygon();
                size_t firstIndex = 0;
                size_t secondIndex = 2;
                if(triangleCounter % 2 == 0){
                    firstIndex = 2;
                    secondIndex = 1;
                }
                currentPolyVertices[0] = currentPolyVertices[firstIndex]; // die letzten beiden? ungekehrte Reihenfolge
                currentPolyVertices[1] = currentPolyVertices[secondIndex];
                currentPolyIndices[0] = currentPolyIndices[firstIndex];
                currentPolyIndices[1] = currentPolyIndices[secondIndex];
                vertexIndex = 2;
                triangleCounter++;
            }
            break;
        }
        default:{ // viereck-strip
            currentPolyVertices[vertexIndex] = v;
            currentPolyIndices[vertexIndex] = 0xFFFF;
            vertexIndex++;
            if(vertexIndex > 3){
                std::swap(currentPolyVertices[2], currentPolyVertices[3]);
                std::swap(currentPolyIndices[2], currentPolyIndices[3]);
                if(polygonInViewport(currentPolygon))
                    pushPolygon();
                currentPolyVertices[0] = currentPolyVertices[3]; // die letzten beiden?
                currentPolyVertices[1] = currentPolyVertices[2];
                currentPolyIndices[0] = currentPolyIndices[3];
                currentPolyIndices[1] = currentPolyIndices[2];
                vertexIndex = 2;
            }
            break;
        }
    }
}

// trivial reject
bool nds::GPU::polygonInViewport(const Polygon &p)
{
    bool allXOver = true;
    bool allXUnder = true;
    bool allYOver = true;
    bool allYUnder = true;
    bool allZOver = geometry.polygonAttr.state.renderFarPlaneIntersectingPolygons > 0;
    bool allZUnder = true;
    bool allWNegative = true;
    for(uint16_t i = 0; i < p.numVertices; i++){
        const auto &v = currentPolyVertices[i];
        allXOver = allXOver && (v.x() > v.w());
        allYOver = allYOver && (v.y() > v.w());
        if(geometry.polygonAttr.state.renderFarPlaneIntersectingPolygons){
            allZOver = allZOver && (v.z() > v.w()); // muss noch geclippt werden
        }
        else{ // ansonsten, wenn IRGENDEIN vertex drüber ist, weg
            allZOver = allZOver || (v.z() > v.w());
        }

        allXUnder = allXUnder && (v.x() < -v.w());
        allYUnder = allYUnder && (v.y() < -v.w());
        allZUnder = allZUnder && (v.z() < -v.w());

        allWNegative = allWNegative && (v.w() < 0);
    }
    return !(allXOver || allXUnder || allYOver || allYUnder || allZOver || allZUnder || allWNegative);
}

OutCode nds::GPU::computeOutCode(const int32_t x, const int32_t y, const int32_t z, const int32_t w)
{
    OutCode code = OUT_INSIDE;

    if(x < -w) code |= OUT_LEFT;
    if(x > w) code |= OUT_RIGHT;
    if(y < -w) code |= OUT_BOTTOM;
    if(y > w) code |= OUT_TOP;
    if(z < -w) code |= OUT_NEAR;
    if(z > w) code |= OUT_FAR;

    return code;
}

int nds::GPU::clipPolygon(std::array<Vertex, 10> &out)
{
    std::array<Vertex, 10> verticesTmp{};
    int numVertices = currentPolygon.numVertices;

    numVertices = clipPlanes(0, currentPolyVertices, out, numVertices);
    numVertices = clipPlanes(1, out, verticesTmp, numVertices);
    numVertices = clipPlanes(2, verticesTmp, out, numVertices);

    return numVertices;
}

Vertex splitEdge(const int planeDim, const Vertex &v1, const Vertex &v2, const int64_t sign){

    Vertex res{};

    const int64_t dv1 = int64_t(v1.w()) - sign * v1.attributes[planeDim];
    const int64_t dv2 = int64_t(v2.w()) - sign * v2.attributes[planeDim];

    const int64_t factor = (dv1 << 16) / (dv1 - dv2);

    for(int i = 0; i < 10; i++){
        res.attributes[i] = v1.attributes[i] + (((v2.attributes[i] - v1.attributes[i]) * factor) >> 16);
    }

    res.attributes[planeDim] = sign * res.w(); // (x/y/z) koordinate exakt auf Rand von Viewport

    return res;
}

// Sutherland-Hodgman
int nds::GPU::clipPlanes(const int planeDim, const std::array<Vertex, 10> &in, std::array<Vertex, 10> &out, int numVertices)
{
    std::array<Vertex, 10> tmp{};
    int current = 0; // Zähler für tmp array
    int prev, next;

    // positive Ebene
    for(int i = 0; i < numVertices; i++){
        prev = i == 0 ? numVertices - 1 : i - 1;
        next = i == numVertices - 1 ? 0 : i + 1;

        Vertex v = in[i];

        if(v.attributes[planeDim] > v.w()){
            if(in[prev].attributes[planeDim] <= in[prev].w()){
                // Neuen Punkt erstellen, interpolieren, einfügen in tmp
                Vertex nv = splitEdge(planeDim, in[prev], v, 1);
                tmp[current++] = nv;
            }
            if(in[next].attributes[planeDim] <= in[next].w()){
                Vertex nv = splitEdge(planeDim, in[next], v, 1);
                tmp[current++] = nv;
            }
        }
        else{
            tmp[current++] = v;
        }
    }

    // in -> tmp -> out
    // in ist jetzt in tmp
    numVertices = current;
    current = 0;

    // negative Ebene
    for(int i = 0; i < numVertices; i++){
        prev = i == 0 ? numVertices - 1 : i - 1;
        next = i == numVertices - 1 ? 0 : i + 1;

        Vertex v = tmp[i];

        if(v.attributes[planeDim] < -v.w()){
            if(tmp[prev].attributes[planeDim] >= -tmp[prev].w()){
                // Neuen Punkt erstellen, interpolieren, einfügen in out
                Vertex nv = splitEdge(planeDim, tmp[prev], v, -1);
                out[current++] = nv;
            }
            if(tmp[next].attributes[planeDim] >= -tmp[next].w()){
                Vertex nv = splitEdge(planeDim, tmp[next], v, -1);
                out[current++] = nv;
            }
        }
        else{
            out[current++] = v;
        }
    }
    numVertices = current;
    return numVertices;
}

void nds::GPU::pushPolygon()
{
    OutCode outcodeOr = 0;
    for(int i = 0; i < currentPolygon.numVertices; i++){
        outcodeOr |= computeOutCode(currentPolyVertices[i].x(), currentPolyVertices[i].y(), currentPolyVertices[i].z(), currentPolyVertices[i].w());
    }

    int64_t determinant = 0;
    {
        const Vertex& v0 = currentPolyVertices[0];
        const Vertex& v1 = currentPolyVertices[1];
        const Vertex& v2 = currentPolyVertices[2];
        determinant = 
          v0.x() * (int64_t(v1.y())*v2.w() - int64_t(v2.y())*v1.w()) 
        - v0.y() * (int64_t(v1.x())*v2.w() - int64_t(v2.x())*v1.w()) 
        + v0.w() * (int64_t(v1.x())*v2.y() - int64_t(v2.x())*v1.y());
    }

    // front back culling
    if(determinant < 0 && !geometry.polygonAttr.state.renderPolyBackside) return;
    if(determinant > 0 && !geometry.polygonAttr.state.renderPolyFrontside) return;

    int origNumVertices = currentPolygon.numVertices;
    int numVertices = currentPolygon.numVertices;

    if(outcodeOr == 0){ // trivial accept (trivial reject in polygonInsideViewport)
        for(uint16_t i = 0; i < numVertices; i++){
            if(currentPolyIndices[i] == 0xFFFF){ // Damit bei Strips die Vertices nicht doppelt drin sind
                currentPolyIndices[i] = vertexRam.size();
                vertexViewportTransform(currentPolyVertices[i]);
                vertexRam.push_back(currentPolyVertices[i]);
            }
            currentPolygon.vertices[i] = currentPolyIndices[i];
        }
    }
    else{
        std::array<Vertex, 10> newVertices{};
        numVertices = clipPolygon(newVertices);
        if(numVertices < 3) return; // nicht gekriegt im trivial reject
        int index = vertexRam.size();
        for(int i = 0; i < numVertices; i++){
            vertexViewportTransform(newVertices[i]);
            vertexRam.push_back(newVertices[i]);
            currentPolygon.vertices[i] = index++;
        }
    }

    currentPolygon.numVertices = numVertices;

    currentPolygon.top = 0;
    currentPolygon.bottom = 0;
    currentPolygon.topY = vertexRam[currentPolygon.vertices[0]].screenY;
    currentPolygon.bottomY = vertexRam[currentPolygon.vertices[0]].screenY;
    for(uint16_t i = 0; i < numVertices; i++){

        if(vertexRam[currentPolygon.vertices[i]].screenY > currentPolygon.bottomY){
            currentPolygon.bottomY = vertexRam[currentPolygon.vertices[i]].screenY;
            currentPolygon.bottom = i;
        }

        if(vertexRam[currentPolygon.vertices[i]].screenY < currentPolygon.topY){
            currentPolygon.topY = vertexRam[currentPolygon.vertices[i]].screenY;
            currentPolygon.top = i;
        }
    }
    currentPolygon.curL = currentPolygon.top;
    currentPolygon.curR = currentPolygon.top;
    currentPolygon.nextL = circularIncrement(currentPolygon.curL, currentPolygon, -1);
    currentPolygon.nextR = circularIncrement(currentPolygon.curR, currentPolygon, 1);

    currentPolygon.attr = geometry.polygonAttr;
    currentPolygon.texParam = geometry.currentTexParam;
    currentPolygon.paletteBaseAddress = geometry.paletteBaseAddress;
    polyRam.push_back(currentPolygon);
    currentPolygon.numVertices = origNumVertices; // reset für nächstes Polygon
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
    geometry.colour.raw = cmd.params[0] & 0xFFFF;
}

void nds::GPU::NORMAL(const FIFOCommand &cmd)
{
}

void nds::GPU::TEXCOORD(const FIFOCommand &cmd)
{
    geometry.nextS = std::bit_cast<int16_t>(uint16_t(cmd.params[0] & 0xFFFF));
    geometry.nextT = std::bit_cast<int16_t>(uint16_t((cmd.params[0] >> 16) & 0xFFFF));
}

void nds::GPU::VTX_16(const FIFOCommand &cmd)
{
    const int32_t x = sign_extend_n_32(cmd.params[0] & 0xFFFF, 16);
    const int32_t y = sign_extend_n_32((cmd.params[0] >> 16) & 0xFFFF, 16);
    const int32_t z = sign_extend_n_32(cmd.params[1] & 0xFFFF, 16);
    geometry.p = vec4(x,y,z);
    Vertex v;
    v.setClipPos(geometry.p);
    transformVertex(v);
    addVertex(v);
}

void nds::GPU::VTX_10(const FIFOCommand &cmd)
{
    const int32_t x = sign_extend_n_32(cmd.params[0] & 0x3FF, 10);
    const int32_t y = sign_extend_n_32((cmd.params[0] >> 10) & 0x3FF, 10);
    const int32_t z = sign_extend_n_32((cmd.params[0] >> 20) & 0x3FF, 10);
    geometry.p = vec4(x << 6,y << 6,z << 6); // Fractional auf 12 bit?
    Vertex v;
    v.setClipPos(geometry.p);
    transformVertex(v);
    addVertex(v);
}

void nds::GPU::VTX_XY(const FIFOCommand &cmd)
{
    const int32_t x = sign_extend_n_32(cmd.params[0] & 0xFFFF, 16);
    const int32_t y = sign_extend_n_32((cmd.params[0] >> 16) & 0xFFFF, 16);
    const int32_t z = geometry.p.z;
    geometry.p = vec4(x,y,z);
    Vertex v;
    v.setClipPos(geometry.p);
    transformVertex(v);
    addVertex(v);
}

void nds::GPU::VTX_XZ(const FIFOCommand &cmd)
{
    const int32_t x = sign_extend_n_32(cmd.params[0] & 0xFFFF, 16);
    const int32_t z = sign_extend_n_32((cmd.params[0] >> 16) & 0xFFFF, 16);
    const int32_t y = geometry.p.y;
    geometry.p = vec4(x,y,z);
    Vertex v;
    v.setClipPos(geometry.p);
    transformVertex(v);
    addVertex(v);
}

void nds::GPU::VTX_YZ(const FIFOCommand &cmd)
{
    const int32_t y = sign_extend_n_32(cmd.params[0] & 0xFFFF, 16);
    const int32_t z = sign_extend_n_32((cmd.params[0] >> 16) & 0xFFFF, 16);
    const int32_t x = geometry.p.x;
    geometry.p = vec4(x,y,z);
    Vertex v;
    v.setClipPos(geometry.p);
    transformVertex(v);
    addVertex(v);
}

void nds::GPU::VTX_DIFF(const FIFOCommand &cmd)
{
    const int32_t x = sign_extend_n_32(cmd.params[0] & 0x3FF, 10);
    const int32_t y = sign_extend_n_32((cmd.params[0] >> 10) & 0x3FF, 10);
    const int32_t z = sign_extend_n_32((cmd.params[0] >> 20) & 0x3FF, 10);

    geometry.p = vec4(geometry.p.x + x,geometry.p.y + y,geometry.p.z + z);
    Vertex v;
    v.setClipPos(geometry.p);
    transformVertex(v);
    addVertex(v);
}

void nds::GPU::POLYGON_ATTR(const FIFOCommand &cmd)
{
    geometry.nextPolygonAttr = POLYGON_ATTR_T{.raw = cmd.params[0]};
}

void nds::GPU::TEXIMAGE_PARAM(const FIFOCommand &cmd)
{
    geometry.currentTexParam.raw = cmd.params[0];
}

void nds::GPU::PLTT_BASE(const FIFOCommand &cmd)
{
    geometry.paletteBaseAddress = cmd.params[0] & 0x1FFF;
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
    triangleCounter = 0;
    if(geometry.primitiveType == 0 || geometry.primitiveType == 2){
        currentPolygon = {.numVertices = 3};
    }
    else{
        currentPolygon = {.numVertices = 4};
    }
    vertexIndex = 0; // in jedem Fall, auch bei Strips
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
    viewportTop = 191 - viewport.state.y2;
    viewportBottom = 191 - viewport.state.y1;
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
