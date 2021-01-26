//
// gx/Renderer.hh
// Copyright (C) 2021 Richard Bradley
//

// TODO - texture wrap settings

#pragma once
#include "Color.hh"
#include "Types.hh"


struct GLFWwindow;
namespace gx {
  class Image;
  class DrawEntry;
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
  virtual TextureID setTexture(TextureID id, const Image& img, int levels,
                               FilterType minFilter, FilterType magFilter) = 0;
  virtual void freeTexture(TextureID id) = 0;
  virtual void setBGColor(float r, float g, float b) = 0;

  // draw methods
  virtual void clearFrame(int width, int height) = 0;
  virtual void draw(const DrawEntry* data, std::size_t dataSize,
                    const Color& col) = 0;
  virtual void renderFrame() = 0;

 protected:
  GLFWwindow* _window = nullptr;
};
