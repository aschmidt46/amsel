#include "controller.h"
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
      state.buttons.up = to;
      if(to && state.buttons.down)
        state.buttons.down = 0;
      break;
    }
    case 1:{ // runter
      state.buttons.down = to;
      if(to && state.buttons.up)
        state.buttons.up = 0;
      break;
    }
    case 2:{ // links
      state.buttons.left = to;
      if(to && state.buttons.right)
        state.buttons.right = 0;
      break;
    }
    case 3:{ // rechts
      state.buttons.right = to;
      if(to && state.buttons.left)
        state.buttons.left = 0;
      break;
    }
    case 4:{ // a
      state.buttons.a = to;
      break;
    }
    case 5:{ // b
      state.buttons.b = to;
      break;
    }
    case 6:{ // start
      state.buttons.start = to;
      break;
    }
    case 7:{ // select
      state.buttons.select = to;
      break;
    }
  }
}

void Controller::clock()
{
  int index = secondary ? 1 : 0;
  mapper->controller[index] = state.raw;
}
