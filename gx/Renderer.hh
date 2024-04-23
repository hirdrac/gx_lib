//
// gx/Renderer.hh
// Copyright (C) 2024 Richard Bradley
//

// TODO: frame stats (draw calls, buffer size)
// TODO: additional mem stats (textures, combined texture size)

#pragma once
#include "DrawList.hh"
#include <initializer_list>
#include <utility>
#include <cstdint>


struct GLFWwindow;

namespace gx {
  class Image;
  class Renderer;


  // Texture Types
  using TextureID = uint32_t;

  enum FilterType {
    FILTER_UNSPECIFIED = 0,
    FILTER_LINEAR,
    FILTER_NEAREST
  };

  enum WrapType {
    WRAP_UNSPECIFIED = 0,
    WRAP_CLAMP_TO_EDGE,
    WRAP_CLAMP_TO_BORDER,
    WRAP_MIRRORED_REPEAT,
    WRAP_REPEAT,
    WRAP_MIRROR_CLAMP_TO_EDGE,
  };

  struct TextureParams {
    FilterType minFilter = FILTER_UNSPECIFIED;
    FilterType magFilter = FILTER_UNSPECIFIED;
    FilterType mipFilter = FILTER_UNSPECIFIED;
    WrapType wrapS = WRAP_UNSPECIFIED;
    WrapType wrapT = WRAP_UNSPECIFIED;
  };


  // Functions
  bool updateTexture(TextureID tid, const Image& img, int levels,
                     const TextureParams& params);
  void freeTexture(TextureID tid);
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
                               const TextureParams& params) = 0;
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
