#include "gpu.h"
#include "../../bus.h"

using namespace nds;

void nds::GPU::clockGeometry()
{
    if(commandFifo.size() > 0 && !fifoHalted){
        // ausführen
        const auto &f = cmdToFunction[commandFifo.front().cmd];
        (this->*f)(commandFifo.front());
        commandFifo.pop();
    }

    if(GXSTAT.state.commandFifoIRQ == 1){
        if(commandFifo.size() < 128){
            bus->setIFSingleBus(21, true, true);
        }
    }
    else if(GXSTAT.state.commandFifoIRQ == 2){
        if(commandFifo.empty()){
            bus->setIFSingleBus(21, true, true);
        }
    }
}

void nds::GPU::NOP(const FIFOCommand &cmd)
{
}

void nds::GPU::MTX_PUSH(const FIFOCommand &cmd)
{
}

void nds::GPU::MTX_POP(const FIFOCommand &cmd)
{
}

void nds::GPU::MTX_STORE(const FIFOCommand &cmd)
{
}

void nds::GPU::MTX_RESTORE(const FIFOCommand &cmd)
{
}

void nds::GPU::MTX_IDENTITY(const FIFOCommand &cmd)
{
}

void nds::GPU::MTX_LOAD_4x4(const FIFOCommand &cmd)
{
}

void nds::GPU::MTX_LOAD_4x3(const FIFOCommand &cmd)
{
}

void nds::GPU::MTX_MULT_4x4(const FIFOCommand &cmd)
{
}

void nds::GPU::MTX_MULT_4x3(const FIFOCommand &cmd)
{
}

void nds::GPU::MTX_MULT_3x3(const FIFOCommand &cmd)
{
}

void nds::GPU::MTX_SCALE(const FIFOCommand &cmd)
{
}

void nds::GPU::MTX_TRANS(const FIFOCommand &cmd)
{
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
}

void nds::GPU::VTX_10(const FIFOCommand &cmd)
{
}

void nds::GPU::VTX_XY(const FIFOCommand &cmd)
{
}

void nds::GPU::VTX_XZ(const FIFOCommand &cmd)
{
}

void nds::GPU::VTX_YZ(const FIFOCommand &cmd)
{
}

void nds::GPU::VTX_DIFF(const FIFOCommand &cmd)
{
}

void nds::GPU::POLYGON_ATTR(const FIFOCommand &cmd)
{
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
}

void nds::GPU::END_VTXS(const FIFOCommand &cmd)
{
}

void nds::GPU::SWAP_BUFFERS(const FIFOCommand &cmd)
{
}

void nds::GPU::VIEWPORT(const FIFOCommand &cmd)
{
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
}
