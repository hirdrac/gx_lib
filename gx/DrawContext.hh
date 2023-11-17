//
// gx/DrawContext.hh
// Copyright (C) 2023 Richard Bradley
//

// TODO: textured roundedRectangle()
// TODO: continuous lines [lineX <vertex count> <v1> <v2> ...]
// TODO: lines as quads
//   - any width supported
//   - multi-line corner types: squared, angled, rounded
// TODO: gradient function instead of set gradient/color points

#pragma once
#include "DrawLayer.hh"
#include "DrawList.hh"
#include "Texture.hh"
#include "Color.hh"
#include "Normal.hh"
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
  const Font* font = nullptr;
  float lineSpacing = 0;  // extra spacing between lines
  float glyphSpacing = 0; // extra spacing between glyphs
  Vec2 advX{1,0};         // dir of next glyph
  Vec2 advY{0,1};         // dir of next line
  Vec2 glyphX{1,0};       // glyph quad sides
  Vec2 glyphY{0,1};
};

class gx::DrawContext
{
 public:
  DrawContext(DrawList& dl) : _data{&dl} { init(); }
  DrawContext(DrawLayer& dl) : DrawContext{dl.entries} { }

  // Low-level data entry
  void clear() { init(); _data->clear(); _dataColor = 0; }
  void reserve(std::size_t n) { _data->reserve(n); }
  [[nodiscard]] std::size_t size() const { return _data->size(); }
  [[nodiscard]] bool empty() const { return _data->empty(); }

  void append(const DrawList& dl) {
    init(); _data->insert(_data->end(), dl.begin(), dl.end()); }
  void append(DrawContext& dc) { append(dc.drawList()); }

  // control/state change
  inline void color(float r, float g, float b, float a = 1.0f);
  inline void color(const Color& c);
  inline void color(RGBA8 c);

  inline void hgradient(float x0, RGBA8 c0, float x1, RGBA8 c1);
  inline void hgradient(float x0, const Color& c0, float x1, const Color& c1);
  inline void vgradient(float y0, RGBA8 c0, float y1, RGBA8 c1);
  inline void vgradient(float y0, const Color& c0, float y1, const Color& c1);

  inline void texture(TextureID tid);
  void texture(const Texture& t) { texture(t.id()); }

  inline void lineWidth(float w);

  void normal(float x, float y, float z) { normal(packNormal(x,y,z)); }
  void normal(const Vec3& n) { normal(packNormal(n)); }
  void normal(uint32_t n) { add(CMD_normal, n); }

  // line drawing
  void line(Vec2 a, Vec2 b);
  void line(const Vec3& a, const Vec3& b);
  void line(const Vertex2C& a, const Vertex2C& b) {
    add(CMD_line2C, a.x, a.y, a.c, b.x, b.y, b.c); }
  void line(const Vertex3C& a, const Vertex3C& b) {
    add(CMD_line3C, a.x, a.y, a.z, a.c, b.x, b.y, b.z, b.c); }

  // poly drawing
  // Triangle  Quad  Rectangle
  //   A--B    A--B    XY--+
  //   | /     | /|    |   H
  //   |/      |/ |    |   |
  //   C       C--D    +-W-+
  void triangle(Vec2 a, Vec2 b, Vec2 c);
  void triangle(const Vec3& a, const Vec3& b, const Vec3& c);
  void triangle(const Vertex2C& a, const Vertex2C& b, const Vertex2C& c) {
    add(CMD_triangle2C, a.x, a.y, a.c, b.x, b.y, b.c, c.x, c.y, c.c); }
  void triangle(const Vertex3C& a, const Vertex3C& b, const Vertex3C& c) {
    add(CMD_triangle3C, a.x, a.y, a.z, a.c, b.x, b.y, b.z, b.c,
        c.x, c.y, c.z, c.c); }
  void triangle(const Vertex2TC& a, const Vertex2TC& b, const Vertex2TC& c) {
    add(CMD_triangle2TC, a.x, a.y, a.s, a.t, a.c,
        b.x, b.y, b.s, b.t, b.c, c.x, c.y, c.s, c.t, c.c); }
  void triangle(const Vertex3TC& a, const Vertex3TC& b, const Vertex3TC& c) {
    add(CMD_triangle3TC, a.x, a.y, a.z, a.s, a.t, a.c,
        b.x, b.y, b.z, b.s, b.t, b.c, c.x, c.y, c.z, c.s, c.t, c.c); }
  void triangle(const Vertex3NTC& a, const Vertex3NTC& b, const Vertex3NTC& c) {
    add(CMD_triangle3NTC,
        a.x, a.y, a.z, packNormal(a.nx, a.ny, a.nz), a.s, a.t, a.c,
        b.x, b.y, b.z, packNormal(b.nx, b.ny, b.nz), b.s, b.t, b.c,
        c.x, c.y, c.z, packNormal(c.nx, c.ny, c.nz), c.s, c.t, c.c); }

  void quad(const Vec2& a, const Vec2& b, const Vec2& c, const Vec2& d);
  void quad(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d);
  void quad(const Vertex2C& a, const Vertex2C& b,
            const Vertex2C& c, const Vertex2C& d) {
    add(CMD_quad2C, a.x, a.y, a.c, b.x, b.y, b.c,
        c.x, c.y, c.c, d.x, d.y, d.c); }
  void quad(const Vertex3C& a, const Vertex3C& b,
            const Vertex3C& c, const Vertex3C& d) {
    add(CMD_quad3C, a.x, a.y, a.z, a.c, b.x, b.y, b.z, b.c,
        c.x, c.y, c.z, c.c, d.x, d.y, d.z, d.c); }
  void quad(const Vertex2TC& a, const Vertex2TC& b,
            const Vertex2TC& c, const Vertex2TC& d) {
    add(CMD_quad2TC, a.x, a.y, a.s, a.t, a.c, b.x, b.y, b.s, b.t, b.c,
        c.x, c.y, c.s, c.t, c.c, d.x, d.y, d.s, d.t, d.c); }
  void quad(const Vertex3TC& a, const Vertex3TC& b,
            const Vertex3TC& c, const Vertex3TC& d) {
    add(CMD_quad3TC, a.x, a.y, a.z, a.s, a.t, a.c, b.x, b.y, b.z, b.s, b.t, b.c,
        c.x, c.y, c.z, c.s, c.t, c.c, d.x, d.y, d.z, d.s, d.t, d.c); }
  void quad(const Vertex3NTC& a, const Vertex3NTC& b,
            const Vertex3NTC& c, const Vertex3NTC& d) {
    add(CMD_quad3NTC,
        a.x, a.y, a.z, packNormal(a.nx, a.ny, a.nz), a.s, a.t, a.c,
        b.x, b.y, b.z, packNormal(b.nx, b.ny, b.nz), b.s, b.t, b.c,
        c.x, c.y, c.z, packNormal(c.nx, c.ny, c.nz), c.s, c.t, c.c,
        d.x, d.y, d.z, packNormal(d.nx, d.ny, d.nz), d.s, d.t, d.c); }

  void rectangle(const Rect& r);
  void rectangle(const Rect& r, Vec2 t0, Vec2 t1);
  void rectangle(const Rect& r, Vec2 t0, Vec2 t1, const Rect& clip);

  // High-level data entry
  void text(const TextFormatting& tf, Vec2 pos, AlignEnum align,
            std::string_view text) {
    _text(tf, pos, align, text, nullptr); }
  void text(const TextFormatting& tf, Vec2 pos, AlignEnum align,
            std::string_view text, const Rect& clip) {
    _text(tf, pos, align, text, &clip); }
  void glyph(const TextFormatting& tf, Vec2 pos, AlignEnum align, int code);

  void circleSector(
    Vec2 center, float radius, float startAngle, float endAngle, int segments);
  void circleSector(Vec2 center, float radius, float startAngle, float endAngle,
                    int segments, RGBA8 innerColor, RGBA8 outerColor);
  void arc(Vec2 center, float radius, float startAngle, float endAngle,
           int segments, float arcWidth);
  void arc(Vec2 center, float radius, float startAngle, float endAngle,
           int segments, float arcWidth, RGBA8 startColor, RGBA8 endColor);
    // NOTE:
    //  * make start and end angles equal for a full circle
    //  * angles are in degrees
    //    angle 0 is (0, -radius) from center, 90 is (radius, 0) from center

  void roundedRectangle(const Rect& r, float curveRadius, int curveSegments);

  void border(const Rect& r, float borderWidth);
  void border(const Rect& r, float borderWidth,
              RGBA8 innerColor, RGBA8 outerColor, RGBA8 fillColor);

  void roundedBorder(const Rect& r,
                     float curveRadius, int curveSegments, float borderWidth);
  void roundedBorder(const Rect& r,
                     float curveRadius, int curveSegments, float borderWidth,
                     RGBA8 innerColor, RGBA8 outerColor, RGBA8 fillColor);

  // Data extraction
  [[nodiscard]] const DrawList& drawList() const { return *_data; }

 private:
  DrawList* _data = nullptr;

  // general properties
  TextureID _lastTexID;
  float _lastLineWidth;

  // color/gradient properties
  float _g0, _g1;                 // x or y gradient coords
  Color _fullcolor0{INIT_NONE};   // full float colors for gradient calc
  Color _fullcolor1{INIT_NONE};
  RGBA8 _color0, _color1;         // current colors
  RGBA8 _dataColor = 0;           // last color set in data
  enum ColorMode { CM_SOLID, CM_HGRADIENT, CM_VGRADIENT };
  ColorMode _colorMode;

  void init() {
    _lastTexID = 0;
    _lastLineWidth = 1.0f;
    _color0 = 0;
    _color1 = 0;
    _colorMode = CM_SOLID;
  }

  template<typename... Args>
  void add(DrawCmd cmd, const Args&... args) {
    const std::initializer_list<DrawEntry> x {cmd, args...};
    _data->insert(_data->end(), x.begin(), x.end());
  }

  void _rectangle(float x, float y, float w, float h);
  void _text(const TextFormatting& tf, Vec2 pos, AlignEnum align,
             std::string_view text, const Rect* clipPtr);
  void _glyph(const Glyph& g, const TextFormatting& tf, Vec2 pos,
              const Rect* clipPtr);
  void _circleSector(
    Vec2 center, float radius, float angle0, float angle1, int segments);
  void _arc(Vec2 center, float radius, float angle0, float angle1,
            int segments, float arcWidth);
  void _arc(Vec2 center, float radius, float angle0, float angle1,
            int segments, float arcWidth,
            RGBA8 innerColor, RGBA8 outerColor, RGBA8 fillColor);

  void _quad(Vec2 a, Vec2 b, Vec2 c, Vec2 d) {
    add(CMD_quad2, a.x, a.y, b.x, b.y, c.x, c.y, d.x, d.y); }

  [[nodiscard]] inline RGBA8 gradientColor(float g) const;
  [[nodiscard]] inline RGBA8 pointColor(Vec2 pt) const;
  [[nodiscard]] inline RGBA8 pointColor(const Vec3& pt) const;
  inline void setColor();
  [[nodiscard]] inline bool checkColor();
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

void gx::DrawContext::color(RGBA8 c)
{
  _colorMode = CM_SOLID;
  _color0 = c;
  _color1 = 0;
}

void gx::DrawContext::hgradient(float x0, RGBA8 c0, float x1, RGBA8 c1)
{
  _colorMode = CM_HGRADIENT;
  _g0 = x0;
  _color0 = c0;
  _fullcolor0 = unpackRGBA8(c0);
  _g1 = x1;
  _color1 = c1;
  _fullcolor1 = unpackRGBA8(c1);
}

void gx::DrawContext::hgradient(
  float x0, const Color& c0, float x1, const Color& c1)
{
  _colorMode = CM_HGRADIENT;
  _g0 = x0;
  _color0 = packRGBA8(c0);
  _fullcolor0 = c0;
  _g1 = x1;
  _color1 = packRGBA8(c1);
  _fullcolor1 = c1;
}

void gx::DrawContext::vgradient(float y0, RGBA8 c0, float y1, RGBA8 c1)
{
  _colorMode = CM_VGRADIENT;
  _g0 = y0;
  _color0 = c0;
  _fullcolor0 = unpackRGBA8(c0);
  _g1 = y1;
  _color1 = c1;
  _fullcolor1 = unpackRGBA8(c1);
}

void gx::DrawContext::vgradient(
  float y0, const Color& c0, float y1, const Color& c1)
{
  _colorMode = CM_VGRADIENT;
  _g0 = y0;
  _color0 = packRGBA8(c0);
  _fullcolor0 = c0;
  _g1 = y1;
  _color1 = packRGBA8(c1);
  _fullcolor1 = c1;
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

gx::RGBA8 gx::DrawContext::gradientColor(float g) const
{
  if (g <= _g0) { return _color0; }
  else if (g >= _g1) { return _color1; }

  const float t = (g - _g0) / (_g1 - _g0);
  return packRGBA8((_fullcolor0 * (1.0f-t)) + (_fullcolor1 * t));
}

gx::RGBA8 gx::DrawContext::pointColor(Vec2 pt) const
{
  switch (_colorMode) {
    default:           return _color0;
    case CM_HGRADIENT: return gradientColor(pt.x);
    case CM_VGRADIENT: return gradientColor(pt.y);
  }
}

gx::RGBA8 gx::DrawContext::pointColor(const Vec3& pt) const
{
  switch (_colorMode) {
    default:           return _color0;
    case CM_HGRADIENT: return gradientColor(pt.x);
    case CM_VGRADIENT: return gradientColor(pt.y);
  }
}

void gx::DrawContext::setColor()
{
  if (_dataColor != _color0) {
    add(CMD_color, _color0);
    _dataColor = _color0;
  }
}

bool gx::DrawContext::checkColor()
{
  if ((_color0 | _color1) == 0) { return false; }
  if (_colorMode == CM_SOLID) { setColor(); }
  return true; // proceed with draw
}
