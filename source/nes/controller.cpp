#include "controller.h"
#include <bit>
#include <iostream>
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
      if(to && state.down)
        state.down = 0;
      break;
    }
    case 1:{ // runter
      state.down = to;
      if(to && state.up)
        state.up = 0;
      break;
    }
    case 2:{ // links
      state.left = to;
      if(to && state.right)
        state.right = 0;
      break;
    }
    case 3:{ // rechts
      state.right = to;
      if(to && state.left)
        state.left = 0;
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
