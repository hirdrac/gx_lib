//
// gx/DrawContext.hh
// Copyright (C) 2021 Richard Bradley
//

// TODO - continuous lines [lineX <vertex count> <v1> <v2> ...]
// TODO - text vertical/horizontal scaling
// TODO - text rotation
// TODO - support color gradiant for primitives other than rectangle

#pragma once
#include "DrawList.hh"
#include "Texture.hh"
#include "Color.hh"
#include "Align.hh"
#include "Types.hh"
#include <vector>
#include <string_view>
#include <initializer_list>


namespace gx {
  // **** Types ****
  struct VertexPT { Vec2 pt, tx; };
  struct VertexPC { Vec2 pt; uint32_t c; };
  struct VertexPTC { Vec2 pt, tx; uint32_t c; };

  class Font;
  class DrawContext;
}


class gx::DrawContext
{
 public:
  DrawContext(DrawList& dl) : _data(&dl) { init(); }

  // Low-level data entry
  void clear() { init(); _data->clear(); }
  void reserve(std::size_t n) { _data->reserve(n); }
  [[nodiscard]] std::size_t size() const { return _data->size(); }

  // control/state change
  inline void color(float r, float g, float b, float a = 1.0f);
  inline void color(const Color& c);
  inline void color(uint32_t c);
  inline void hgradiant(float x0, uint32_t c0, float x1, uint32_t c1);
  inline void vgradiant(float y0, uint32_t c0, float y1, uint32_t c1);

  inline void lineWidth(float w);

  inline void texture(TextureID tid);
  void texture(const Texture& t) { texture(t.id()); }

  // line drawing
  void line(Vec2 p0, Vec2 p1) { add(CMD_line, p0.x, p0.y, p1.x, p1.y); }

  // 2D poly drawing
  // Triangle  Quad  Rectangle
  //   0--1    0--1    XY--+
  //   | /     | /|    |   H
  //   |/      |/ |    |   |
  //   2       2--3    +-W-+

  void triangle(Vec2 p0, Vec2 p1, Vec2 p2) {
    add(CMD_triangle, p0.x, p0.y, p1.x, p1.y, p2.x, p2.y); }
  void triangle(const VertexPT& v0, const VertexPT& v1, const VertexPT& v2) {
    add(CMD_triangleT, v0.pt.x, v0.pt.y, v0.tx.x, v0.tx.y,
        v1.pt.x, v1.pt.y, v1.tx.x, v1.tx.y,
        v2.pt.x, v2.pt.y, v2.tx.x, v2.tx.y); }
  void triangle(const VertexPC& v0, const VertexPC& v1, const VertexPC& v2) {
    add(CMD_triangleC, v0.pt.x, v0.pt.y, v0.c,
        v1.pt.x, v1.pt.y, v1.c, v2.pt.x, v2.pt.y, v2.c); }
  void triangle(const VertexPTC& v0, const VertexPTC& v1, const VertexPTC& v2) {
    add(CMD_triangleTC, v0.pt.x, v0.pt.y, v0.tx.x, v0.tx.y, v0.c,
        v1.pt.x, v1.pt.y, v1.tx.x, v1.tx.y, v1.c,
        v2.pt.x, v2.pt.y, v2.tx.x, v2.tx.y, v2.c); }

  void quad(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3) {
    add(CMD_quad, p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, p3.x, p3.y); }
  void quad(const VertexPT& v0, const VertexPT& v1,
            const VertexPT& v2, const VertexPT& v3) {
    add(CMD_quadT, v0.pt.x, v0.pt.y, v0.tx.x, v0.tx.y,
        v1.pt.x, v1.pt.y, v1.tx.x, v1.tx.y,
        v2.pt.x, v2.pt.y, v2.tx.x, v2.tx.y,
        v3.pt.x, v3.pt.y, v3.tx.x, v3.tx.y); }
  void quad(const VertexPC& v0, const VertexPC& v1,
            const VertexPC& v2, const VertexPC& v3) {
    add(CMD_quadC, v0.pt.x, v0.pt.y, v0.c, v1.pt.x, v1.pt.y, v1.c,
        v2.pt.x, v2.pt.y, v2.c, v3.pt.x, v3.pt.y, v3.c); }
  void quad(const VertexPTC& v0, const VertexPTC& v1,
            const VertexPTC& v2, const VertexPTC& v3) {
    add(CMD_quadTC, v0.pt.x, v0.pt.y, v0.tx.x, v0.tx.y, v0.c,
        v1.pt.x, v1.pt.y, v1.tx.x, v1.tx.y, v1.c,
        v2.pt.x, v2.pt.y, v2.tx.x, v2.tx.y, v2.c,
        v3.pt.x, v3.pt.y, v3.tx.x, v3.tx.y, v3.c); }

  void rectangle(float x, float y, float w, float h);
  void rectangle(float x, float y, float w, float h, Vec2 t0, Vec2 t1);
  void rectangle(float x, float y, float w, float h, Vec2 t0, Vec2 t1,
                 const Rect& clip);

  // High-level data entry
  void text(const Font& f, float x, float y, AlignEnum align, int spacing,
	    std::string_view text) {
    _text(f, x, y, align, spacing, text, nullptr); }
  void text(const Font& f, float x, float y, AlignEnum align, int spacing,
            std::string_view text, const Rect& clip) {
    _text(f, x, y, align, spacing, text, &clip); }

  // Data extraction
  [[nodiscard]] const DrawList& drawList() const { return *_data; }

 private:
  DrawList* _data = nullptr;
  uint32_t _lastColor;
  float _lastLineWidth;
  TextureID _lastTexID;

  float _g0, _g1;         // x or y gradiant coords
  uint32_t _c0, _c1;      // gradiant colors (packed)
  Color _color0, _color1; // gradiant colors (full)

  enum ColorMode { CM_SOLID, CM_HGRADIANT, CM_VGRADIANT };
  ColorMode _colorMode;

  void init() {
    _lastColor = 0xffffffff;
    _lastLineWidth = 1.0f;
    _lastTexID = 0;
    _colorMode = CM_SOLID;
  }

  template<typename... Args>
  void add(DrawCmd cmd, const Args&... args) {
    std::initializer_list<DrawEntry> x = {cmd, args...};
    _data->insert(_data->end(), x.begin(), x.end());
  }

  void _text(const Font& f, float x, float y, AlignEnum align, int spacing,
             std::string_view text, const Rect* clipPtr);

  [[nodiscard]] inline uint32_t gradiantColor(float g) const;
};


// **** Inline Implementations ****
void gx::DrawContext::color(float r, float g, float b, float a)
{
  color(packRGBA8(r, g, b, a));
}

void gx::DrawContext::color(const Color& c)
{
  color(packRGBA8(c));
}

void gx::DrawContext::color(uint32_t c)
{
  _colorMode = CM_SOLID;
  if (c != _lastColor) {
    _lastColor = c;
    add(CMD_color, c);
  }
}

void gx::DrawContext::hgradiant(float x0, uint32_t c0, float x1, uint32_t c1)
{
  if (c0 == c1) {
    color(c0);
  } else {
    _colorMode = CM_HGRADIANT;
    _g0 = x0;
    _c0 = c0;
    _color0 = unpackRGBA8(c0);
    _g1 = x1;
    _c1 = c1;
    _color1 = unpackRGBA8(c1);
  }
}

void gx::DrawContext::vgradiant(float y0, uint32_t c0, float y1, uint32_t c1)
{
  if (c0 == c1) {
    color(c0);
  } else {
    _colorMode = CM_HGRADIANT;
    _g0 = y0;
    _c0 = c0;
    _color0 = unpackRGBA8(c0);
    _g1 = y1;
    _c1 = c1;
    _color1 = unpackRGBA8(c1);
  }
}

void gx::DrawContext::lineWidth(float w)
{
  if (w != _lastLineWidth) {
    _lastLineWidth = w;
    add(CMD_lineWidth, w);
  }
}

void gx::DrawContext::texture(TextureID tid)
{
  if (tid != _lastTexID) {
    _lastTexID = tid;
    add(CMD_texture, tid);
  }
}

uint32_t gx::DrawContext::gradiantColor(float g) const
{
  if (g <= _g0) { return _c0; }
  else if (g >= _g1) { return _c1; }

  float t = (g - _g0) / (_g1 - _g0);
  return packRGBA8((_color0 * (1.0f-t)) + (_color1 * t));
}
