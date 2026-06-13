#include "gba_implementation.h"

GbaImplementation::GbaImplementation(const char *path)
{
    gba = std::make_shared<gba::GBA>(path);
}

GbaImplementation::GbaImplementation(std::vector<uint8_t> &rom)
{
    gba = std::make_shared<gba::GBA>(rom);
}

void GbaImplementation::load(const char *path)
{
    gba = std::make_shared<gba::GBA>(path);
}

void GbaImplementation::clock() {
    gba->clock();
}

void GbaImplementation::clockUntilSampleReady() {
    gba->clockUntilSampleReady();
}

const float *GbaImplementation::accessFramebuffer()
{
    return gba->accessFramebuffer();
}

bool GbaImplementation::frameIsReady()
{
    return gba->hasFrame();
}

bool GbaImplementation::audioSampleReady()
{
    return gba->hasSample();
}

std::pair<double, double> GbaImplementation::getSample()
{
    return gba->getSample();
}

bool GbaImplementation::isLoaded()
{
    return true;
}

float GbaImplementation::getX()
{
    return 240.0f;
}

float GbaImplementation::getY()
{
    return 160.0f;
}

void GbaImplementation::setController1Key(bool gamepad, int key, int action) {}

void GbaImplementation::setController2Key(bool gamepad, int key, int action) {}

bool GbaImplementation::canSave()
{
    return false;
}

std::vector<uint8_t> GbaImplementation::getSaveData()
{
    return std::vector<uint8_t>();
}

void GbaImplementation::addClock() {
    gba->addClock();
}

void GbaImplementation::setHalt(bool val) {
    gba->setHalt(val);
}

bool GbaImplementation::isHalted()
{
    return gba->isHalted();
}

void GbaImplementation::produceDisassembly(bool val) {}

int GbaImplementation::addressBytes() {
    return 4;
}

std::pair<std::string, std::vector<int>> GbaImplementation::getCurrentDisassembly()
{
    return gba->getNextInstructions();
}

std::pair<std::string, std::vector<int>> GbaImplementation::getOldDisassembly()
{
    return gba->getPrevInstructions();
}

std::vector<uint16_t> GbaImplementation::addBreakpoint(uint16_t bp)
{
    return std::vector<uint16_t>();
}

std::vector<uint16_t> GbaImplementation::removeBreakpoint(uint16_t bp)
{
    return std::vector<uint16_t>();
}

std::vector<std::string> GbaImplementation::addBreakpointOP(std::string bp)
{
    return std::vector<std::string>();
}

std::vector<std::string> GbaImplementation::removeBreakpointOP(std::string bp)
{
    return std::vector<std::string>();
}

std::string GbaImplementation::getText(uint16_t addr)
{
    return std::string();
}

std::string GbaImplementation::getOpcodeName(size_t index)
{
    return std::string();
}

uint8_t GbaImplementation::readCpuBus(uint16_t addr)
{
    return 0;
}

void GbaImplementation::displayRegisters() {}
