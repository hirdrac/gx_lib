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
  virtual void renderFrame() = 0;


  // helper functions
  void setBGColor(const Color& c) { setBGColor(c.r, c.g, c.b); }

  void setModColor(uint32_t c) {
    _drawBuffer.push_back(CMD_modColor);
    _drawBuffer.push_back(c);
  }

  void setModColor(const Color& c) { setModColor(packRGBA8(c)); }

  void setTransform(const Mat4& view, const Mat4& proj) {
    _drawBuffer.reserve(_drawBuffer.size() + 33);
    _drawBuffer.push_back(CMD_transform);
    for (auto x : view) { _drawBuffer.push_back(x); }
    for (auto x : proj) { _drawBuffer.push_back(x); }
  }

  void draw(const DrawEntry* data, std::size_t dataSize) {
    if (_drawCap != _lastCap) {
      _drawBuffer.push_back(CMD_capabilities);
      _drawBuffer.push_back(_drawCap);
      _lastCap = _drawCap;
    }
    _drawBuffer.insert(_drawBuffer.end(), data, data + dataSize);
    _changed = true;
  }

  void draw(const DrawList& dl) { draw(dl.data(), dl.size()); }

  template<typename Drawable>
  void draw(const Drawable& d) { draw(d.drawList()); }

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
  std::vector<DrawEntry> _drawBuffer;
  int _maxTextureSize = 0;
  int _drawCap = INIT_CAPABILITIES;
  int _lastCap = -1;
  bool _changed = true;
};
