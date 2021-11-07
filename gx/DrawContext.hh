//
// gx/DrawContext.hh
// Copyright (C) 2021 Richard Bradley
//

// TODO - support color gradiant for primitives other than rectangle
// TODO - textured roundedRectangle()
// TODO - continuous lines [lineX <vertex count> <v1> <v2> ...]
// TODO - lines as quads:
//        * any width supported
//        * multi-line corner types: squared, angled, rounded

#pragma once
#include "DrawList.hh"
#include "Texture.hh"
#include "Color.hh"
#include "Align.hh"
#include "Types.hh"
#include <string_view>
#include <initializer_list>


namespace gx {
  class Font;
  struct Glyph;
  class DrawContext;
  struct TextFormatting;
}

struct gx::TextFormatting
{
  float spacing = 0;
  Vec2 advX = {1,0};    // dir of next character
  Vec2 advY = {0,1};    // dir of next line
  Vec2 glyphX = {1,0};  // glyph quad sides
  Vec2 glyphY = {0,1};
};

class gx::DrawContext
{
 public:
  DrawContext(DrawList& dl) : _data{&dl} { init(); }

  // Low-level data entry
  void clear() { init(); _data->clear(); }
  void reserve(std::size_t n) { _data->reserve(n); }
  [[nodiscard]] std::size_t size() const { return _data->size(); }
  void append(const DrawList& dl) {
    _data->insert(_data->end(), dl.begin(), dl.end()); }

  // control/state change
  inline void color(float r, float g, float b, float a = 1.0f);
  inline void color(const Color& c);
  inline void color(uint32_t c);

  inline void hgradiant(float x0, uint32_t c0, float x1, uint32_t c1);
  inline void hgradiant(float x0, const Color& c0, float x1, const Color& c1);
  inline void vgradiant(float y0, uint32_t c0, float y1, uint32_t c1);
  inline void vgradiant(float y0, const Color& c0, float y1, const Color& c1);

  inline void lineWidth(float w);

  inline void texture(TextureID tid);
  void texture(const Texture& t) { texture(t.id()); }

  // line drawing
  void line(Vec2 a, Vec2 b) {
    add(CMD_line2, a.x, a.y, b.x, b.y); }
  void line(const Vec3& a, const Vec3& b) {
    add(CMD_line3, a.x, a.y, a.z, b.x, b.y, b.z); }

  // poly drawing
  // Triangle  Quad  Rectangle
  //   A--B    A--B    XY--+
  //   | /     | /|    |   H
  //   |/      |/ |    |   |
  //   C       C--D    +-W-+
  void triangle(Vec2 a, Vec2 b, Vec2 c) {
    add(CMD_triangle2, a.x, a.y, b.x, b.y, c.x, c.y); }
  void triangle(const Vec3& a, const Vec3& b, const Vec3& c) {
    add(CMD_triangle3, a.x, a.y, a.z, b.x, b.y, b.z, c.x, c.y, c.z); }

  void triangle(const Vertex2C& a, const Vertex2C& b, const Vertex2C& c) {
    add(CMD_triangle2C, a.x, a.y, a.c, b.x, b.y, b.c, c.x, c.y, c.c); }
  void triangle(const Vertex3C& a, const Vertex3C& b, const Vertex3C& c) {
    add(CMD_triangle3C, a.x, a.y, a.z, a.c, b.x, b.y, b.z, b.c,
        c.x, c.y, c.z, c.c); }

  void triangle(const Vertex2T& a, const Vertex2T& b, const Vertex2T& c) {
    add(CMD_triangle2T, a.x, a.y, a.s, a.t,
        b.x, b.y, b.s, b.t, c.x, c.y, c.s, c.t); }
  void triangle(const Vertex3T& a, const Vertex3T& b, const Vertex3T& c) {
    add(CMD_triangle3T, a.x, a.y, a.z, a.s, a.t,
        b.x, b.y, b.z, b.s, b.t, c.x, c.y, c.z, c.s, c.t); }

  void triangle(const Vertex2TC& a, const Vertex2TC& b, const Vertex2TC& c) {
    add(CMD_triangle2TC, a.x, a.y, a.s, a.t, a.c,
        b.x, b.y, b.s, b.t, b.c, c.x, c.y, c.s, c.t, c.c); }
  void triangle(const Vertex3TC& a, const Vertex3TC& b, const Vertex3TC& c) {
    add(CMD_triangle3TC, a.x, a.y, a.z, a.s, a.t, a.c,
        b.x, b.y, b.z, b.s, b.t, b.c, c.x, c.y, c.z, c.s, c.t, c.c); }

  void quad(Vec2 a, Vec2 b, Vec2 c, Vec2 d) {
    add(CMD_quad2, a.x, a.y, b.x, b.y, c.x, c.y, d.x, d.y); }
  void quad(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d) {
    add(CMD_quad3, a.x, a.y, a.z, b.x, b.y, b.z, c.x, c.y, c.z, d.x, d.y, d.z); }

  void quad(const Vertex2C& a, const Vertex2C& b,
            const Vertex2C& c, const Vertex2C& d) {
    add(CMD_quad2C, a.x, a.y, a.c, b.x, b.y, b.c,
        c.x, c.y, c.c, d.x, d.y, d.c); }
  void quad(const Vertex3C& a, const Vertex3C& b,
            const Vertex3C& c, const Vertex3C& d) {
    add(CMD_quad3C, a.x, a.y, a.z, a.c, b.x, b.y, b.z, b.c,
        c.x, c.y, c.z, c.c, d.x, d.y, d.z, d.c); }

  void quad(const Vertex2T& a, const Vertex2T& b,
            const Vertex2T& c, const Vertex2T& d) {
    add(CMD_quad2T, a.x, a.y, a.s, a.t, b.x, b.y, b.s, b.t,
        c.x, c.y, c.s, c.t, d.x, d.y, d.s, d.t); }
  void quad(const Vertex3T& a, const Vertex3T& b,
            const Vertex3T& c, const Vertex3T& d) {
    add(CMD_quad3T, a.x, a.y, a.z, a.s, a.t, b.x, b.y, b.z, b.s, b.t,
        c.x, c.y, c.z, c.s, c.t, d.x, d.y, d.z, d.s, d.t); }

  void quad(const Vertex2TC& a, const Vertex2TC& b,
            const Vertex2TC& c, const Vertex2TC& d) {
    add(CMD_quad2TC, a.x, a.y, a.s, a.t, a.c, b.x, b.y, b.s, b.t, b.c,
        c.x, c.y, c.s, c.t, c.c, d.x, d.y, d.s, d.t, d.c); }
  void quad(const Vertex3TC& a, const Vertex3TC& b,
            const Vertex3TC& c, const Vertex3TC& d) {
    add(CMD_quad3TC, a.x, a.y, a.z, a.s, a.t, a.c, b.x, b.y, b.z, b.s, b.t, b.c,
        c.x, c.y, c.z, c.s, c.t, c.c, d.x, d.y, d.z, d.s, d.t, d.c); }

  void rectangle(float x, float y, float w, float h);
  void rectangle(const Rect& r) { rectangle(r.x, r.y, r.w, r.h); }
  void rectangle(float x, float y, float w, float h, Vec2 t0, Vec2 t1);
  void rectangle(const Rect& r, Vec2 t0, Vec2 t1) {
    rectangle(r.x, r.y, r.w, r.h, t0, t1); }
  void rectangle(float x, float y, float w, float h, Vec2 t0, Vec2 t1,
                 const Rect& clip);
  void rectangle(const Rect& r, Vec2 t0, Vec2 t1, const Rect& clip) {
    rectangle(r.x, r.y, r.w, r.h, t0, t1, clip); }

  // High-level data entry
  void text(const Font& f, const TextFormatting& tf, float x, float y,
            AlignEnum align, std::string_view text) {
    _text(f, tf, x, y, align, text, nullptr); }
  void text(const Font& f, const TextFormatting& tf, float x, float y,
            AlignEnum align, std::string_view text, const Rect& clip) {
    _text(f, tf, x, y, align, text, &clip); }

  void circleSector(
    Vec2 center, float radius, float startAngle, float endAngle, int segments);
  void circleSector(Vec2 center, float radius, float startAngle, float endAngle,
                    int segments, uint32_t color0, uint32_t color1);
    // NOTE:
    //  * make start and end angles equal for a full circle
    //  * angles are in degrees
    //    angle 0 is (0, -radius) from center, 90 is (radius, 0) from center

  void roundedRectangle(float x, float y, float w, float h,
                        float curveRadius, int curveSegments);

  // Data extraction
  [[nodiscard]] const DrawList& drawList() const { return *_data; }

 private:
  DrawList* _data = nullptr;

  // general properties
  TextureID _lastTexID;
  float _lastLineWidth;

  // color/gradiant properties
  float _g0, _g1;         // x or y gradiant coords
  uint32_t _c0, _c1;      // gradiant colors (packed)
  Color _color0, _color1; // gradiant colors (full)
  uint32_t _lastColor;
  enum ColorMode { CM_SOLID, CM_HGRADIANT, CM_VGRADIANT };
  ColorMode _colorMode;


  void init() {
    _lastTexID = 0;
    _lastLineWidth = 1.0f;
    _lastColor = 0;
    _colorMode = CM_SOLID;
  }

  template<typename... Args>
  void add(DrawCmd cmd, const Args&... args) {
    const std::initializer_list<DrawEntry> x {cmd, args...};
    _data->insert(_data->end(), x.begin(), x.end());
  }

  void _text(const Font& f, const TextFormatting& tf, float x, float y,
             AlignEnum align, std::string_view text, const Rect* clipPtr);
  void _glyph(const Glyph& g, const TextFormatting& tf, Vec2 cursor,
              const Rect* clipPtr);
  void _rect(float x, float y, float w, float h) {
    add(CMD_rectangle, x, y, x + w, y + h); }

  [[nodiscard]] inline uint32_t gradiantColor(float g) const;

  [[nodiscard]] uint32_t pointColor(Vec2 pt) const {
    switch (_colorMode) {
      default:           return _lastColor;
      case CM_HGRADIANT: return gradiantColor(pt.x);
      case CM_VGRADIANT: return gradiantColor(pt.y);
    }
  }
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
  _colorMode = CM_HGRADIANT;
  _g0 = x0;
  _c0 = c0;
  _color0 = unpackRGBA8(c0);
  _g1 = x1;
  _c1 = c1;
  _color1 = unpackRGBA8(c1);
}

void gx::DrawContext::hgradiant(
  float x0, const Color& c0, float x1, const Color& c1)
{
  _colorMode = CM_HGRADIANT;
  _g0 = x0;
  _c0 = packRGBA8(c0);
  _color0 = c0;
  _g1 = x1;
  _c1 = packRGBA8(c1);
  _color1 = c1;
}

void gx::DrawContext::vgradiant(float y0, uint32_t c0, float y1, uint32_t c1)
{
  _colorMode = CM_HGRADIANT;
  _g0 = y0;
  _c0 = c0;
  _color0 = unpackRGBA8(c0);
  _g1 = y1;
  _c1 = c1;
  _color1 = unpackRGBA8(c1);
}

void gx::DrawContext::vgradiant(
  float y0, const Color& c0, float y1, const Color& c1)
{
  _colorMode = CM_HGRADIANT;
  _g0 = y0;
  _c0 = packRGBA8(c0);
  _color0 = c0;
  _g1 = y1;
  _c1 = packRGBA8(c1);
  _color1 = c1;
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

  const float t = (g - _g0) / (_g1 - _g0);
  return packRGBA8((_color0 * (1.0f-t)) + (_color1 * t));
}
