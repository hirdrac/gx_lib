//
// gx/Renderer.cc
// Copyright (C) 2021 Richard Bradley
//

#include "Renderer.hh"
#include "System.hh"
#include <GLFW/glfw3.h>


gx::Renderer::~Renderer()
{
  if (_window && glfwInitStatus()) {
    glfwDestroyWindow(_window);
  }
}

void gx::Renderer::clearFrame(int width, int height)
{
  _width = width;
  _height = height;
  _layers.clear();
  _transforms.clear();
  _changed = true;
}
