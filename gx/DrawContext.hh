//
// gx/DrawContext.hh
// Copyright (C) 2024 Richard Bradley
//

// TODO: textured roundedRectangle()
// TODO: continuous lines [lineX <vertex count> <v1> <v2> ...]
// TODO: lines as quads
//   - any width supported
//   - multi-line corner types: squared, angled, rounded
// TODO: gradient function instead of set gradient/color points

#pragma once
#include "DrawList.hh"
#include "Renderer.hh"
#include "Color.hh"
#include "Normal.hh"
#include "Align.hh"
#include "TextFormat.hh"
#include "Types.hh"
#include <string_view>
#include <initializer_list>


namespace gx {
  struct Glyph;
  class DrawContext;

  struct Vertex2C {
    float x, y; uint32_t c;

    Vertex2C() = default;
    constexpr Vertex2C(Vec2 pos, uint32_t col)
      : x{pos.x}, y{pos.y}, c{col} { }
    constexpr Vertex2C(float px, float py, uint32_t col)
      : x{px}, y{py}, c{col} { }
  };

  struct Vertex2T { float x, y, s, t; };
  struct Vertex2TC { float x, y, s, t; uint32_t c; };

  struct Vertex3C { float x, y, z; uint32_t c; };
  struct Vertex3T { float x, y, z, s, t; };
  struct Vertex3TC { float x, y, z, s, t; uint32_t c; };

  struct Vertex3TCN { float x,y,z,s,t; uint32_t c, n; };
}

class gx::DrawContext
{
 public:
  DrawContext(std::nullptr_t) = delete;
  explicit DrawContext(DrawList* dl) : _data{dl} { init(); }
  explicit DrawContext(DrawList& dl) : DrawContext{&dl} { }

  // Low-level data entry
  void clearList() { init(); _data->clear(); }
  void append(const DrawList& dl) {
    init(); _data->insert(_data->end(), dl.begin(), dl.end()); }

  // Context state change (reset for every DrawList)
  inline void color(float r, float g, float b, float a = 1.0f);
  inline void color(const Color& c);
  inline void color(RGBA8 c);

  inline void hgradient(float x0, RGBA8 c0, float x1, RGBA8 c1);
  inline void hgradient(float x0, const Color& c0, float x1, const Color& c1);
  inline void vgradient(float y0, RGBA8 c0, float y1, RGBA8 c1);
  inline void vgradient(float y0, const Color& c0, float y1, const Color& c1);

  inline void normal(float x, float y, float z);
  inline void normal(const Vec3& n);
  inline void normal(uint32_t n);

  inline void texture(TextureID tid);
  void texture(const TextureHandle& h) { texture(h.id()); }

  // Render state change (persists across different DrawLists)
  void lineWidth(float w) { add(CMD_lineWidth, w); }

  void modColor(float r, float g, float b, float a = 1.0f) {
    add(CMD_modColor, packRGBA8(r, g, b, a)); }
  void modColor(const Color& c) { add(CMD_modColor, packRGBA8(c)); }
  void modColor(RGBA8 c) { add(CMD_modColor, c); }

  void capabilities(int32_t c) { add(CMD_capabilities, c); }

  // Camera
  void camera(const Mat4& viewT, const Mat4& projT) {
    add(CMD_camera, viewT, projT); }
  void cameraReset() { add(CMD_cameraReset); }

  // Lighting
  void light(Vec3 pos, RGBA8 ambient, RGBA8 diffuse) {
    add(CMD_light, pos.x, pos.y, pos.z, ambient, diffuse); }

  // View
  void viewport(int x, int y, int w, int h) {
    add(CMD_viewport, x, y, w, h); }
  void viewportFull() {
    add(CMD_viewportFull); }
  void clearView(float r, float g, float b) {
    clearView(packRGBA8(r,g,b,1.0f)); }
  void clearView(const Color& c) {
    clearView(packRGBA8(c)); }
  void clearView(RGBA8 c) {
    add(CMD_clear, c); }

  // Line drawing
  void line(Vec2 a, Vec2 b);
  void line(const Vec3& a, const Vec3& b);
  void line(const Vertex2C& a, const Vertex2C& b) {
    add(CMD_line2C, a.x, a.y, a.c, b.x, b.y, b.c); }
  void line(const Vertex3C& a, const Vertex3C& b) {
    add(CMD_line3C, a.x, a.y, a.z, a.c, b.x, b.y, b.z, b.c); }

  void lineStart(Vec2 a);
  void lineTo(Vec2 a);
  void lineStart(const Vec3& a);
  void lineTo(const Vec3& a);
  void lineStart(const Vertex2C& a) {
    add(CMD_lineStart2C, a.x, a.y, a.c); }
  void lineTo(const Vertex2C& a) {
    add(CMD_lineTo2C, a.x, a.y, a.c); }
  void lineStart(const Vertex3C& a) {
    add(CMD_lineStart3C, a.x, a.y, a.z, a.c); }
  void lineTo(const Vertex3C& a) {
    add(CMD_lineTo3C, a.x, a.y, a.z, a.c); }

  // Poly drawing
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
  void triangle(const Vertex3TCN& a, const Vertex3TCN& b, const Vertex3TCN& c) {
    add(CMD_triangle3TCN,
        a.x, a.y, a.z, a.s, a.t, a.c, a.n,
        b.x, b.y, b.z, b.s, b.t, b.c, b.n,
        c.x, c.y, c.z, c.s, c.t, c.c, c.n); }

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
  void quad(const Vertex3TCN& a, const Vertex3TCN& b,
            const Vertex3TCN& c, const Vertex3TCN& d) {
    add(CMD_quad3TCN,
        a.x, a.y, a.z, a.s, a.t, a.c, a.n,
        b.x, b.y, b.z, b.s, b.t, b.c, b.n,
        c.x, c.y, c.z, c.s, c.t, c.c, c.n,
        d.x, d.y, d.z, d.s, d.t, d.c); }

  void rectangle(const Rect& r);
  void rectangle(const Rect& r, Vec2 t0, Vec2 t1);
  void rectangle(const Rect& r, Vec2 t0, Vec2 t1, const Rect& clip);

  // Text drawing
  void text(const TextFormat& tf, Vec2 pos, AlignEnum align,
            std::string_view text) {
    _text(tf, pos, align, text, nullptr); }
  void text(const TextFormat& tf, Vec2 pos, AlignEnum align,
            std::string_view text, const Rect& clip) {
    _text(tf, pos, align, text, &clip); }
  void glyph(const TextFormat& tf, Vec2 pos, AlignEnum align, int code);

  // High-level constructed primitives
  void circleSector(
    Vec2 center, float radius, float startAngle, float endAngle, int segments);
  void circleSectorShaded(
    Vec2 center, float radius, float startAngle, float endAngle,
    int segments, RGBA8 innerColor, RGBA8 outerColor);
  void arc(
    Vec2 center, float radius, float startAngle, float endAngle,
    int segments, float arcWidth);
  void arcShaded(
    Vec2 center, float radius, float startAngle, float endAngle,
    int segments, float arcWidth, RGBA8 startColor, RGBA8 endColor);
    // NOTE:
    //  * make start and end angles equal for a full circle
    //  * angles are in degrees
    //    angle 0 is (0, -radius) from center, 90 is (radius, 0) from center

  void roundedRectangle(const Rect& r, float curveRadius, int curveSegments);

  void border(const Rect& r, float borderWidth);
  void borderShaded(
    const Rect& r, float borderWidth,
    RGBA8 innerColor, RGBA8 outerColor, RGBA8 fillColor);

  void roundedBorder(
    const Rect& r, float curveRadius, int curveSegments, float borderWidth);
  void roundedBorderShaded(
    const Rect& r, float curveRadius, int curveSegments, float borderWidth,
    RGBA8 innerColor, RGBA8 outerColor, RGBA8 fillColor);

  // Data extraction
  [[nodiscard]] const DrawList& drawList() const { return *_data; }

 private:
  DrawList* _data = nullptr;

  // general properties
  TextureID _lastTexID;
  uint32_t _lastNormal;

  // color/gradient properties
  float _g0, _g1;                 // x or y gradient coords
  Color _fullcolor0{INIT_NONE};   // full float colors for gradient calc
  Color _fullcolor1{INIT_NONE};
  RGBA8 _color0, _color1;         // current colors
  RGBA8 _dataColor;               // last color set in data
  enum ColorMode { CM_SOLID, CM_HGRADIENT, CM_VGRADIENT };
  ColorMode _colorMode;

  void init() {
    _lastTexID = 0;
    _lastNormal = 0;
    _color0 = 0;
    _color1 = 0;
    _dataColor = 0;
    _colorMode = CM_SOLID;
  }

  void add(DrawCmd cmd, const Mat4& m1, const Mat4& m2) {
    _data->reserve(_data->size() + 33);
    _data->push_back(cmd);
    _data->insert(_data->end(), m1.begin(), m1.end());
    _data->insert(_data->end(), m2.begin(), m2.end());
  }

  template<class... Args>
  void add(DrawCmd cmd, const Args&... args) {
    const std::initializer_list<DrawEntry> x {cmd, args...};
    _data->insert(_data->end(), x.begin(), x.end());
  }

  void _rectangle(float x, float y, float w, float h);
  void _text(const TextFormat& tf, Vec2 pos, AlignEnum align,
             std::string_view text, const Rect* clipPtr);
  void _glyph(const Glyph& g, const TextFormat& tf, Vec2 pos,
              const Rect* clipPtr);
  void _circleSector(
    Vec2 center, float radius, float angle0, float angle1, int segments);
  void _arc(Vec2 center, float radius, float angle0, float angle1,
            int segments, float arcWidth);
  void _arcShaded(Vec2 center, float radius, float angle0, float angle1,
                  int segments, float arcWidth,
                  RGBA8 innerColor, RGBA8 outerColor, RGBA8 fillColor);

  void _quad(Vec2 a, Vec2 b, Vec2 c, Vec2 d) {
    add(CMD_quad2, a.x, a.y, b.x, b.y, c.x, c.y, d.x, d.y); }

  [[nodiscard]] RGBA8 gradientColor(float g) const;
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

void gx::DrawContext::normal(float x, float y, float z)
{
  normal(packNormal(x,y,z));
}

void gx::DrawContext::normal(const Vec3& n)
{
  normal(packNormal(n));
}

void gx::DrawContext::normal(uint32_t n)
{
  if (n != _lastNormal) {
    _lastNormal = n;
    add(CMD_normal, n);
  }
}

void gx::DrawContext::texture(TextureID tid)
{
  if (tid != _lastTexID) {
    _lastTexID = tid;
    add(CMD_texture, tid);
  }
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
    _dataColor = _color0;
    add(CMD_color, _color0);
  }
}

bool gx::DrawContext::checkColor()
{
  if ((_color0 | _color1) == 0) { return false; }
  if (_colorMode == CM_SOLID) { setColor(); }
  return true; // proceed with draw
}
