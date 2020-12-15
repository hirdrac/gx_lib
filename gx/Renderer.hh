//
// gx/Renderer.hh
// Copyright (C) 2020 Richard Bradley
//

// TODO - texture wrap/filter settings

#pragma once
#include "Color.hh"
#include "Types.hh"


struct GLFWwindow;
namespace gx {
  class Image;
  class DrawList;
  class Renderer;
}

class gx::Renderer
{
 public:
  virtual ~Renderer();

  // GLFW window methods
  GLFWwindow* window() { return _window; }

  // setup methods
  virtual void setWindowHints(bool debug) = 0;
  virtual bool init(GLFWwindow* win) = 0;

  virtual int maxTextureSize() = 0;
  virtual int setTexture(
    int id, const Image& img, FilterType minFilter, FilterType magFilter) = 0;
  virtual void freeTexture(int id) = 0;
  virtual void setBGColor(float r, float g, float b) = 0;

  // draw methods
  virtual void clearFrame(int width, int height) = 0;
  virtual void draw(const DrawList& dl, const Color& col) = 0;
  virtual void renderFrame() = 0;

 protected:
  GLFWwindow* _window = nullptr;
};
