#include "controller.h"
#include <bit>
#include <iostream>
#include <GLFW/glfw3.h>
#include <assert.h>

void Controller::setKey(bool gamepad, int key, int v)
{
  auto c = secondary ? &globalConfig.controller2 : &globalConfig.controller1;
  // Das ist nicht die tollste Lösung, reicht aber für diesen einfachen Anwendungsfall mit nur 8 Tasten
  if(gamepad){
    for(int i = 0; i < 8; i++){
      if((*c)[i].second == key){
        setAddressOf(i, v);
        return;
      }
    }
  }
  else{ // Tastatur
    for(int i = 0; i < 8; i++){
      if((*c)[i].first == key){
        setAddressOf(i, v);
        return;
      }
    }
  }
}

void Controller::setAddressOf(int i, int to)
{
  switch(i){
    case 0:{ // hoch
      state.up = to;
      break;
    }
    case 1:{ // runter
      state.down = to;
      break;
    }
    case 2:{ // links
      state.left = to;
      break;
    }
    case 3:{ // rechts
      state.right = to;
      break;
    }
    case 4:{ // a
      state.a = to;
      break;
    }
    case 5:{ // b
      state.b = to;
      break;
    }
    case 6:{ // start
      state.start = to;
      break;
    }
    case 7:{ // select
      state.select = to;
      break;
    }
  }
}

void Controller::clock()
{
  int index = secondary ? 1 : 0;
  mapper->controller[index] = state.raw;
}
