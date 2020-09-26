//
// Renderer.cc
// Copyright (C) 2020 Richard Bradley
//

#include "Renderer.hh"
#include <GLFW/glfw3.h>


gx::Renderer::~Renderer()
{
  if (_window) {
    glfwDestroyWindow(_window);
  }
}
