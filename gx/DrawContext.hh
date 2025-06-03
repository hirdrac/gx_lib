//
// gx/DrawContext.hh
// Copyright (C) 2025 Richard Bradley
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
#include "Style.hh"
#include "Types.hh"
#include <string_view>


namespace gx {
  class DrawContext;
}

class gx::DrawContext
{
 public:
  DrawContext(std::nullptr_t) = delete;
  explicit DrawContext(DrawList* dl);
  explicit DrawContext(DrawList& dl) : DrawContext{&dl} { }

  // Low-level data entry
  void clearList() { init(); _dl->clear(); }
  void append(const DrawList& dl) { init(); _dl->append(dl); }

  // Context state change (reset for every DrawList)
  void color(float r, float g, float b, float a = 1.0f) {
    color(packRGBA8(r, g, b, a)); }
  void color(const Color& c) { color(packRGBA8(c)); }
  inline void color(RGBA8 c);

  inline void hgradient(float x0, RGBA8 c0, float x1, RGBA8 c1);
  inline void hgradient(float x0, const Color& c0, float x1, const Color& c1);
  inline void vgradient(float y0, RGBA8 c0, float y1, RGBA8 c1);
  inline void vgradient(float y0, const Color& c0, float y1, const Color& c1);

  void normal(float x, float y, float z) { normal(packNormal(x,y,z)); }
  void normal(const Vec3& n) { normal(packNormal(n)); }
  inline void normal(uint32_t n);

  inline void texture(TextureID tid);
  void texture(const TextureHandle& h) { texture(h.id()); }

  // Render state change (persists across different DrawLists)
  void lineWidth(float w) { _dl->lineWidth(w); }

  void modColor(float r, float g, float b, float a = 1.0f) {
    modColor(packRGBA8(r, g, b, a)); }
  void modColor(const Color& c) { modColor(packRGBA8(c)); }
  void modColor(RGBA8 c) { _dl->modColor(c); }

  void capabilities(int32_t c) { _dl->capabilities(c); }

  // Camera
  void camera(const Mat4& viewT, const Mat4& projT) {
    _dl->camera(viewT, projT); }
  void cameraReset() { _dl->cameraReset(); }

  // Lighting
  void light(Vec3 pos, RGBA8 ambient, RGBA8 diffuse) {
    _dl->light(pos, ambient, diffuse); }

  // View
  void viewport(int x, int y, int w, int h) {
    _dl->viewport(x, y, w, h); }
  void viewportFull() {
    _dl->viewportFull(); }
  void clearView(float r, float g, float b) {
    clearView(packRGBA8(r,g,b,1.0f)); }
  void clearView(const Color& c) {
    clearView(packRGBA8(c)); }
  void clearView(RGBA8 c) {
    _dl->clearView(c); }

  // Line drawing
  void line(Vec2 a, Vec2 b);
  void line(const Vec3& a, const Vec3& b);
  void line(const Vertex2C& a, const Vertex2C& b) {
    _dl->line2C(a, b); }
  void line(const Vertex3C& a, const Vertex3C& b) {
    _dl->line3C(a, b); }

  void lineStart(Vec2 a);
  void lineTo(Vec2 a);
  void lineStart(const Vec3& a);
  void lineTo(const Vec3& a);
  void lineStart(const Vertex2C& a) {
    _dl->lineStart2C(a); }
  void lineTo(const Vertex2C& a) {
    _dl->lineTo2C(a); }
  void lineStart(const Vertex3C& a) {
    _dl->lineStart3C(a); }
  void lineTo(const Vertex3C& a) {
    _dl->lineTo3C(a); }

  // Poly drawing
  // Triangle  Quad  Rectangle
  //   A--B    A--B    XY--+
  //   | /     | /|    |   H
  //   |/      |/ |    |   |
  //   C       C--D    +-W-+
  void triangle(Vec2 a, Vec2 b, Vec2 c);
  void triangle(const Vec3& a, const Vec3& b, const Vec3& c);
  void triangle(const Vertex2C& a, const Vertex2C& b, const Vertex2C& c) {
    _dl->triangle2C(a, b, c); }
  void triangle(const Vertex3C& a, const Vertex3C& b, const Vertex3C& c) {
    _dl->triangle3C(a, b, c); }
  void triangle(const Vertex2TC& a, const Vertex2TC& b, const Vertex2TC& c) {
    _dl->triangle2TC(a, b, c); }
  void triangle(const Vertex3TC& a, const Vertex3TC& b, const Vertex3TC& c) {
    _dl->triangle3TC(a, b, c); }
  void triangle(const Vertex3TCN& a, const Vertex3TCN& b, const Vertex3TCN& c) {
    _dl->triangle3TCN(a, b, c); }

  void quad(const Vec2& a, const Vec2& b, const Vec2& c, const Vec2& d);
  void quad(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d);
  void quad(const Vertex2C& a, const Vertex2C& b,
            const Vertex2C& c, const Vertex2C& d) {
    _dl->quad2C(a, b, c, d); }
  void quad(const Vertex3C& a, const Vertex3C& b,
            const Vertex3C& c, const Vertex3C& d) {
    _dl->quad3C(a, b, c, d); }
  void quad(const Vertex2TC& a, const Vertex2TC& b,
            const Vertex2TC& c, const Vertex2TC& d) {
    _dl->quad2TC(a, b, c, d); }
  void quad(const Vertex3TC& a, const Vertex3TC& b,
            const Vertex3TC& c, const Vertex3TC& d) {
    _dl->quad3TC(a, b, c, d); }
  void quad(const Vertex3TCN& a, const Vertex3TCN& b,
            const Vertex3TCN& c, const Vertex3TCN& d) {
    _dl->quad3TCN(a, b, c, d); }

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

  void shape(const Rect& r, const Style& style);

  // Data extraction
  [[nodiscard]] const DrawList& drawList() const { return *_dl; }

 private:
  DrawList* _dl = nullptr;

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

  void _rectangle(float x, float y, float w, float h);
  void _text(const TextFormat& tf, Vec2 pos, AlignEnum align,
             std::string_view text, const Rect* clipPtr);
  void _glyph(const Glyph& g, const TextFormat& tf, Vec2 pos,
              const Rect* clipPtr, float altWidth = 0);
  void _circleSector(
    Vec2 center, float radius, float angle0, float angle1, int segments);
  void _arc(Vec2 center, float radius, float angle0, float angle1,
            int segments, float arcWidth);
  void _arcShaded(Vec2 center, float radius, float angle0, float angle1,
                  int segments, float arcWidth,
                  RGBA8 innerColor, RGBA8 outerColor, RGBA8 fillColor);

  [[nodiscard]] RGBA8 gradientColor(float g) const;
  [[nodiscard]] inline RGBA8 pointColor(Vec2 pt) const;
  [[nodiscard]] inline RGBA8 pointColor(const Vec3& pt) const;
  inline void setColor();
  inline bool checkColor();
};


// **** Inline Implementations ****
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

void gx::DrawContext::normal(uint32_t n)
{
  if (n != _lastNormal) {
    _lastNormal = n;
    _dl->normal(n);
  }
}

void gx::DrawContext::texture(TextureID tid)
{
  if (tid != _lastTexID) {
    _lastTexID = tid;
    _dl->texture(tid);
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
    _dl->color(_color0);
  }
}

bool gx::DrawContext::checkColor()
{
  if ((_color0 | _color1) == 0) { return false; }
  if (_colorMode == CM_SOLID) { setColor(); }
  return true; // proceed with draw
}
