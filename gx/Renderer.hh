//
// gx/Renderer.hh
// Copyright (C) 2022 Richard Bradley
//

// TODO: texture wrap settings
// TODO: frame stats (draw calls, buffer size, layers)
// TODO: additional mem stats (textures, combined texture size)
// TODO: optional clear depth for layer
// TODO: clear draw data from single layer/layer range

#pragma once
#include "DrawLayer.hh"
#include "Color.hh"
#include "Types.hh"
#include <initializer_list>


struct GLFWwindow;
namespace gx {
  class Image;
  struct DrawEntry;
  class Renderer;
}

class gx::Renderer
{
 public:
  Renderer() = default;
  virtual ~Renderer();

  // prevent copy/assignment/move
  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;

  // setup methods
  virtual void setWindowHints(bool debug) = 0;
  virtual bool init(GLFWwindow* win) = 0;

  // texture methods
  virtual TextureID setTexture(TextureID id, const Image& img, int levels,
                               FilterType minFilter, FilterType magFilter) = 0;
  virtual void freeTexture(TextureID id) = 0;

  // draw methods
  virtual void draw(
    int width, int height, std::initializer_list<DrawLayer*> dl) = 0;
  virtual void renderFrame() = 0;

  // general accessors
  [[nodiscard]] GLFWwindow* window() { return _window; }
  [[nodiscard]] int maxTextureSize() const { return _maxTextureSize; }

 protected:
  GLFWwindow* _window = nullptr;
  int _maxTextureSize = 0;

  [[nodiscard]] TextureID newTextureID();
};


namespace gx {
  bool updateTexture(TextureID id, const Image& img, int levels,
                     FilterType minFilter, FilterType magFilter);
  void freeTexture(TextureID tid);
}
