#include "controller.h"
#include <bit>
#include <iostream>
#include <GLFW/glfw3.h>
#include <assert.h>


void Controller::setKey(int key, int v)
{
    switch(key){
    case GLFW_KEY_UP:
      state.up = v;
      break;
    case GLFW_KEY_DOWN:
      state.down = v;
      break;
    case GLFW_KEY_LEFT:
      state.left = v;
      break;
    case GLFW_KEY_RIGHT:
      state.right = v;
      break;
    case GLFW_KEY_S:
      state.a = v;
      break;
    case GLFW_KEY_A:
      state.b = v;
      break;
    case GLFW_KEY_ENTER:
      state.start = v;
      break;
    case GLFW_KEY_BACKSPACE:
      state.select = v;
      break;
    default:
      break;
  }
}

void Controller::clock()
{
    mapper->controller[0] = state.raw;
}
