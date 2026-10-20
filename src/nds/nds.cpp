#include "nds.h"
#include "framework/global.h"
#include <iostream>
#include "framework/stringlib.h"

using namespace nds;

void nds::NDS::checkBios()
{
    if(bus->bus7->getBiosSize() == 0 || bus->bus9->getBiosSize() == 0){
        executionAllowed = false;
        sendMessage = true;
    }
}

nds::NDS::NDS(const std::vector<uint8_t> &bytes, const StartInfo &info)
{
    bus = std::make_shared<SharedBus>();
    bus->init();
    bus->bus7->spi.setFirmware(info.firmware);
    loadBios7(info.bios7);
    loadBios9(info.bios9);
    checkBios();
    bus->loadCartHLE(bytes);
    // bus->loadCart(bytes);

    if(info.haltAt.has_value()){
        bus->stepBack = true;
        bus->haltAt = info.haltAt.value();
    }
}

void nds::NDS::loadBios7(const std::vector<uint8_t> &content)
{
    bus->bus7->loadBios(content);
}

void nds::NDS::loadBios9(const std::vector<uint8_t> &content)
{
    bus->bus9->loadBios(content);
}

void nds::NDS::loadFirmware(const std::vector<uint8_t> &content)
{
    bus->bus7->spi.setFirmware(content);
}

uint8_t *nds::NDS::accessFramebuffer()
{
    return (uint8_t*)bus->accessFramebuffer();
}

void nds::NDS::clock()
{
    // 33 MHz
    bus->clock();

    audioTime += audioTimePerGBAClock;
    if(audioTime >= audioTimePerSystemSample){
        audioTime -= audioTimePerSystemSample;
        auto [l, r] = bus->getSample();
        audioSampleL = l;
        audioSampleR = r;
        audioSampleReady = true;
    }
}

void nds::NDS::clockUntilSampleReady()
{
    if(sendMessage){
        MessageStruct m = {.type = MT_ERROR, .title = {}, .content = locale.getTranslation(NDSMissingBios)};
        messageQueue.enqueue(m);
        sendMessage = false;
    }
    if(executionAllowed){
        while(!hasSample()){
            clock();
        }
    }
}

bool nds::NDS::hasFrame()
{
    if(!bus->isHalted() || bus->steps > 0)
        return bus->lcd->hasFrame();
    else return true;
}

bool nds::NDS::hasSample()
{
    bool tmp = audioSampleReady;
    audioSampleReady = false;
    return tmp;
}

bool nds::NDS::canSave()
{
    return bus->gameCart.canSave();
}

std::vector<uint8_t> nds::NDS::getSaveData()
{
    return bus->gameCart.getSaveData();
}

size_t nds::NDS::getSaveSize()
{
    return bus->gameCart.getSaveSize();
}

void nds::NDS::loadSave(const std::vector<uint8_t> &saveData)
{
    bus->gameCart.loadSave(saveData);
}

std::pair<float, float> nds::NDS::getSample()
{
    return {audioSampleL, audioSampleR};
}

void nds::NDS::press(int i)
{
    bus->press(i);
}

void nds::NDS::release(int i)
{
    bus->release(i);
}

void nds::NDS::setTouch(float x, float y)
{
    bus->bus7->spi.touch.setTouch(x,y);
}

void nds::NDS::releaseTouch()
{
    bus->bus7->spi.touch.releaseTouch();
}

void nds::NDS::setDebug(bool to)
{
    this->bus->setDebug(to);
}

void nds::NDS::setHalt(bool to)
{
    bus->setHalt(to);
}

bool nds::NDS::isHalted() {
    return bus->isHalted();
}

void nds::NDS::addClock() {
    this->bus->addStep();
}

std::pair<std::string, std::vector<int>> nds::NDS::getNextInstructions() {
    return bus->getNextInstructions();
}

std::pair<std::string, std::vector<int>> nds::NDS::getPrevInstructions() {
    return bus->getPrevInstructions();
}

std::vector<std::string> nds::NDS::removeBreakpointOP(std::string bp) {
    return bus->removeBreakpointOP(bp);
}

std::vector<std::string> nds::NDS::addBreakpointOP(std::string bp) {
    return bus->addBreakpointOP(bp);
}

nds::CpuRegisterState nds::NDS::getRegs() {
    return bus->getRegs();
}

std::vector<std::string> nds::NDS::getStack() {
    return bus->getStack();
}

std::string nds::NDS::getMode() {
    return bus->getMode();
}
std::string nds::NDS::getState(){
    return bus->getState();
}

std::vector<uint64_t> nds::NDS::addBreakpoint(uint64_t bp) {
  return bus->addBreakpoint(bp);
}

std::vector<uint64_t> nds::NDS::removeBreakpoint(uint64_t bp) {
  return bus->removeBreakpoint(bp);
}

std::string nds::NDS::getDisassembly(uint64_t code)
{
    return bus->getDisassembly(code);
}

uint64_t nds::NDS::readBus(uint64_t addr)
{
    return bus->watchARM7 ? bus->bus7->readByteFromWide(addr) : bus->bus9->readByteFromWide(addr);
}

std::tuple<std::string, std::string, std::string> nds::NDS::getLastTransaction()
{
    return bus->getLastTransaction();
}