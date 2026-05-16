#include "cgb_implementation.h"
#include "../framework/global.h"

void CgbImplementation::setAddressOf(int i, int to)
{
    switch(i){
        case 0:{ // hoch
          if(to){
            press_joypad(this->console, 2);
          }
          else{
            release_joypad(this->console, 2);
          }
          break;
        }
        case 1:{ // runter
          if(to){
            press_joypad(this->console, 3);
          }
          else{
            release_joypad(this->console, 3);
          }
          break;
        }
        case 2:{ // links
          if(to){
            press_joypad(this->console, 1);
          }
          else{
            release_joypad(this->console, 1);
          }
          break;
        }
        case 3:{ // rechts
          if(to){
            press_joypad(this->console, 0);
          }
          else{
            release_joypad(this->console, 0);
          }
          break;
        }
        case 4:{ // a
          if(to){
            press_button(this->console, 0);
          }
          else{
            release_button(this->console, 0);
          }
          break;
        }
        case 5:{ // b
          if(to){
            press_button(this->console, 1);
          }
          else{
            release_button(this->console, 1);
          }
          break;
        }
        case 6:{ // start
          if(to){
            press_button(this->console, 3);
          }
          else{
            release_button(this->console, 3);
          }
          break;
        }
        case 7:{ // select
          if(to){
            press_button(this->console, 2);
          }
          else{
            release_button(this->console, 2);
          }
          break;
        }
    }
}

CgbImplementation::CgbImplementation(const char *path) : console(new_cgb(path))
{
}

void CgbImplementation::load(const char *path)
{
    this->console = new_cgb(path);
}

void CgbImplementation::clock()
{
    cgb_clock(this->console);
}

void CgbImplementation::clockUntilSampleReady()
{
    cgb_clock_until_samle_ready(this->console);
}

const float *CgbImplementation::accessFramebuffer()
{
    return access_framebuffer(this->console);
}

bool CgbImplementation::frameIsReady()
{
    return has_frame(this->console);
}

bool CgbImplementation::audioSampleReady()
{
    return audio_sample_ready(this->console);
}

bool CgbImplementation::shouldChangeTitle()
{
    return false;
}

std::string CgbImplementation::getTitle()
{
    return std::string();
}

std::pair<double, double> CgbImplementation::getSample()
{
    auto res = get_stereo(this->console);
    return {res.left, res.right};
}

bool CgbImplementation::isLoaded()
{
    return true;
}

float CgbImplementation::getX()
{
    return 160.0f;
}

float CgbImplementation::getY()
{
    return 144.0f;
}

void CgbImplementation::setController1Key(bool gamepad, int key, int action)
{
    // TODO
    auto c = &globalConfig.controller1;
    if(gamepad){
      for(int i = 0; i < 8; i++){
        if((*c)[i].second == key){
          setAddressOf(i, action);
          return;
        }
      }
    }
    else{ // Tastatur
      for(int i = 0; i < 8; i++){
        if((*c)[i].first == key){
          setAddressOf(i, action);
          return;
        }
      }
    }
}

void CgbImplementation::setController2Key(bool gamepad, int key, int action)
{
    // this->setController1Key(gamepad, key, action);
}

std::pair<std::string, std::vector<int>> CgbImplementation::getCurrentDisassembly()
{
    return std::pair<std::string, std::vector<int>>();
}

std::pair<std::string, std::vector<int>> CgbImplementation::getOldDisassembly()
{
    return std::pair<std::string, std::vector<int>>();
}

std::vector<uint16_t> CgbImplementation::addBreakpoint(uint16_t bp)
{
    return std::vector<uint16_t>();
}

std::vector<uint16_t> CgbImplementation::removeBreakpoint(uint16_t bp)
{
    return std::vector<uint16_t>();
}

std::vector<std::string> CgbImplementation::addBreakpointOP(std::string bp)
{
    return std::vector<std::string>();
}

std::vector<std::string> CgbImplementation::removeBreakpointOP(std::string bp)
{
    return std::vector<std::string>();
}

std::string CgbImplementation::getText(uint16_t addr)
{
    return std::string();
}

std::string CgbImplementation::getOpcodeName(size_t index)
{
    return std::string();
}

uint8_t CgbImplementation::readCpuBus(uint16_t addr)
{
    return 0;
}


void CgbImplementation::displayRegisters()
{
}
