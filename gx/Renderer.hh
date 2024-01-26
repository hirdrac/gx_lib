//
// gx/Renderer.hh
// Copyright (C) 2024 Richard Bradley
//

// TODO: texture wrap settings
// TODO: frame stats (draw calls, buffer size)
// TODO: additional mem stats (textures, combined texture size)

#pragma once
#include "DrawList.hh"
#include "Types.hh"
#include <initializer_list>


struct GLFWwindow;
namespace gx {
  class Image;
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
  virtual bool init(GLFWwindow* win) = 0;
  virtual bool setSwapInterval(int interval) = 0;
  virtual bool setFramebufferSize(int width, int height) = 0;

  // texture methods
  virtual TextureID setTexture(TextureID id, const Image& img, int levels,
                               FilterType minFilter, FilterType magFilter) = 0;
  virtual void freeTexture(TextureID id) = 0;

  // draw methods
  virtual void draw(std::initializer_list<const DrawList*> dl) = 0;
  virtual void renderFrame(int64_t usecTime) = 0;

  // general accessors
  [[nodiscard]] GLFWwindow* window() { return _window; }
  [[nodiscard]] int framebufferWidth() const { return _fbWidth; }
  [[nodiscard]] int framebufferHeight() const { return _fbHeight; }
  [[nodiscard]] std::pair<int,int> framebufferDimensions() const {
    return {_fbWidth, _fbHeight}; }
  [[nodiscard]] int maxTextureSize() const { return _maxTextureSize; }
  [[nodiscard]] int swapInterval() const { return _swapInterval; }
  [[nodiscard]] int frameRate() const { return _frameRate; }

 protected:
  GLFWwindow* _window = nullptr;
  int _fbWidth = 0;
  int _fbHeight = 0;
  int _maxTextureSize = 0;
  int _swapInterval = 1;
  int _frameRate = 0;

  [[nodiscard]] TextureID newTextureID();
};


namespace gx {
  bool updateTexture(TextureID tid, const Image& img, int levels,
                     FilterType minFilter, FilterType magFilter);
  void freeTexture(TextureID tid);
}
