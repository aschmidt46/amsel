#include "gba.h"

gba::GBA::GBA(const char *path, const char* biosPath)
{
    bus = std::make_shared<Bus>(path, biosPath);
    bus->init();
}

gba::GBA::GBA(const std::vector<uint8_t> &bytes)
{
    bus = std::make_shared<Bus>(bytes);
    bus->init();
}

uint8_t *gba::GBA::accessFramebuffer()
{
    return (uint8_t*)bus->accessFramebuffer();
}

void gba::GBA::clock() {
    bus->clock();

    audioTime += audioTimePerGBAClock;
    if(audioTime >= audioTimePerSystemSample){
        audioTime -= audioTimePerSystemSample;
        audioSampleL = 0;//apu->getSample(true);
        audioSampleR = 0;//apu->getSample(true);
        audioSampleReady = true;
    }

}

void gba::GBA::clockUntilSampleReady() {
    while(!hasSample()){
        clock();
    }
}

bool gba::GBA::hasFrame(){
    return bus->hasFrame();
}

bool gba::GBA::hasSample(){
    bool tmp = audioSampleReady;
    audioSampleReady = false;
    return tmp;
}

std::pair<float, float> gba::GBA::getSample(){
    return {0,0};
}

void gba::GBA::press(int i){
    this->bus->press(i);
}

void gba::GBA::release(int i){
    this->bus->release(i);
}

void gba::GBA::loadBios(const std::vector<uint8_t> &content) {
    this->bus->loadBios(content);
}

void gba::GBA::setHalt(bool to) {
    bus->setHalt(to);
}

bool gba::GBA::isHalted() {
    return bus->isHalted();
}

void gba::GBA::addClock() {
    this->bus->addStep();
}

std::pair<std::string, std::vector<int>> gba::GBA::getNextInstructions() {
    return bus->getNextInstructions();
}

std::pair<std::string, std::vector<int>> gba::GBA::getPrevInstructions() {
    return bus->getPrevInstructions();
}

std::vector<std::string> gba::GBA::removeBreakpointOP(std::string bp) {
    return bus->removeBreakpointOP(bp);
}

std::vector<std::string> gba::GBA::addBreakpointOP(std::string bp) {
    return bus->addBreakpointOP(bp);
}

gba::CpuRegisterState gba::GBA::getRegs() {
    return bus->getRegs();
}

std::vector<std::string> gba::GBA::getStack() {
    return bus->getStack();
}

std::string gba::GBA::getMode() {
    return bus->getMode();
}
std::string gba::GBA::getState(){
    return bus->getState();
}

std::vector<uint64_t> gba::GBA::addBreakpoint(uint64_t bp) {
  return bus->addBreakpoint(bp);
}

std::vector<uint64_t> gba::GBA::removeBreakpoint(uint64_t bp) {
  return bus->removeBreakpoint(bp);
}

std::string gba::GBA::getDisassembly(uint64_t code)
{
    return bus->getDisassembly(code);
}

uint64_t gba::GBA::readBus(uint64_t addr)
{
    return bus->readByte(addr);
}

std::tuple<std::string, std::string, std::string> gba::GBA::getLastTransaction()
{
    return bus->getLastTransaction();
}
