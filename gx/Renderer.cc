//
// gx/Renderer.cc
// Copyright (C) 2021 Richard Bradley
//

#include "Renderer.hh"
#include "System.hh"
#include "Camera.hh"
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

void gx::Renderer::setScreenOrthoProjection(int layer)
{
  Mat4 orthoT;
  calcOrthoProjection(float(_width), float(_height), orthoT);
  setTransform(layer, Mat4Identity, orthoT);
}

void gx::Renderer::setOrthoProjection(int layer, float width, float height)
{
  Mat4 orthoT;
  calcOrthoProjection(width, height, orthoT);
  setTransform(layer, Mat4Identity, orthoT);
}
