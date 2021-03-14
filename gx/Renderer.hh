//
// gx/Renderer.hh
// Copyright (C) 2021 Richard Bradley
//

// TODO - texture wrap settings
// TODO - frame stats (draw calls, buffer size)

#pragma once
#include "Color.hh"
#include "DrawList.hh"
#include "Types.hh"


struct GLFWwindow;
namespace gx {
  class Image;
  struct DrawEntry;
  class Renderer;
}

class gx::Renderer
{
 public:
  virtual ~Renderer();

  // setup methods
  virtual void setWindowHints(bool debug) = 0;
  virtual bool init(GLFWwindow* win) = 0;
  virtual void setBGColor(float r, float g, float b) = 0;

  // texture methods
  virtual TextureID setTexture(TextureID id, const Image& img, int levels,
                               FilterType minFilter, FilterType magFilter) = 0;
  virtual void freeTexture(TextureID id) = 0;

  // draw methods
  virtual void clearFrame(int width, int height) = 0;
  virtual void setTransform(const Mat4& view, const Mat4& proj) = 0;
  virtual void draw(const DrawEntry* data, std::size_t dataSize,
                    const Color& col) = 0;
  virtual void renderFrame() = 0;


  // helper functions
  void setBGColor(const Color& c) { setBGColor(c.r, c.g, c.b); }

  void draw(const DrawList& dl) {
    draw(dl.data(), dl.size(), WHITE); }
  void draw(const DrawList& dl, const Color& modColor) {
    draw(dl.data(), dl.size(), modColor); }

  template<typename Drawable>
  void draw(const Drawable& d) { draw(d.drawList()); }

  template<typename Drawable, typename... Args>
  void draw(const Drawable& d, const Args&... args) {
    draw(d.drawList(), args...); }

  // general accessors
  [[nodiscard]] GLFWwindow* window() { return _window; }
  [[nodiscard]] int maxTextureSize() const { return _maxTextureSize; }

  // draw capabilities
  enum CapabilityEnum {
    BLEND = 1, DEPTH_TEST = 2 };
  static constexpr int INIT_CAPABILITIES = BLEND;

  void enable(CapabilityEnum c) { _drawCap |= c; }
  void disable(CapabilityEnum c) { _drawCap &= ~int(c); }
  void setCapabilities(int c) { _drawCap = c; }

 protected:
  GLFWwindow* _window = nullptr;
  int _maxTextureSize = 0;
  int _drawCap = 0; // capabilities to use for next draw call
};
