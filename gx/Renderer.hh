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
#include "Color.hh"
#include "DrawList.hh"
#include "Types.hh"
#include <map>


struct GLFWwindow;
namespace gx {
  class Image;
  struct DrawEntry;
  class Renderer;

  enum CapabilityEnum {
    BLEND = 1,
    DEPTH_TEST = 2,
    CULL_CW = 4,
    CULL_CCW = 8,
  };
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
  virtual void renderFrame() = 0;


  // general functions
  void clearFrame(int width, int height);

  void setBGColor(float r, float g, float b) { _bgColor.set(r,g,b); }
  void setBGColor(const Color& c) { _bgColor.set(c.r, c.g, c.b); }

  void setClearDepth(int layer, bool enable) {
    _layers[layer].clearDepth = enable; }
  void setModColor(int layer, uint32_t c) {
    _layers[layer].modColor = c; }
  void setModColor(int layer, const Color& c) {
    setModColor(layer, packRGBA8(c)); }

  void setTransform(int layer, const Mat4& view, const Mat4& proj) {
    Layer& lyr = _layers[layer];
    lyr.view = view;
    lyr.proj = proj;
    lyr.transformSet = true;
  }

  void setScreenOrthoProjection(int layer);
  void setOrthoProjection(int layer, float width, float height);

  void draw(int layer, const DrawEntry* data, std::size_t dataSize);
  void draw(int layer, const DrawList& dl) {
    draw(layer, dl.data(), dl.size()); }
  void draw(const DrawList& dl) {
    draw(0, dl.data(), dl.size()); }

  // general accessors
  [[nodiscard]] GLFWwindow* window() { return _window; }
  [[nodiscard]] int maxTextureSize() const { return _maxTextureSize; }

  // draw capabilities
  static constexpr int INIT_CAPABILITIES = BLEND;
  void setCapabilities(int layer, int c) { _layers[layer].cap = c; }

 protected:
  GLFWwindow* _window = nullptr;
  int _maxTextureSize = 0;
  int _width = 0, _height = 0;
  Vec3 _bgColor{};
  bool _changed = true;

  struct Layer {
    DrawList drawData;
    uint32_t modColor = 0xffffffff;
    int cap = -1;
    Mat4 view, proj;
    bool transformSet = false;
    bool clearDepth = false;
  };
  std::map<int,Layer> _layers;

  [[nodiscard]] TextureID newTextureID();
};


namespace gx {
  bool updateTexture(TextureID id, const Image& img, int levels,
                     FilterType minFilter, FilterType magFilter);
  void freeTexture(TextureID tid);
}
