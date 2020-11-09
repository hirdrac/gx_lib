//
// gx/DrawList.hh
// Copyright (C) 2020 Richard Bradley
//

// TODO - vertical/horizontal color gradiant
// TODO - clipping region
// TODO - continuous lines [lineX <vertex count> <v1> <v2> ...]
// TODO - text vertical/horizontal scaling
// TODO - text rotation
// TODO - textured triangle/quad

#pragma once
#include "Texture.hh"
#include "Color.hh"
#include "types.hh"
#include <vector>
#include <string_view>
#include <initializer_list>
#include <cstdint>


namespace gx {
  // **** Types ****
  enum AlignEnum {
    ALIGN_UNSPECIFIED = 0,

    // vertical alignments
    ALIGN_TOP = 1<<0,
    ALIGN_BOTTOM = 1<<1,
    ALIGN_VCENTER = ALIGN_TOP | ALIGN_BOTTOM,
    ALIGN_VJUSTIFY = 1<<2,

    // horizontal alignments
    ALIGN_LEFT = 1<<3,
    ALIGN_RIGHT = 1<<4,
    ALIGN_HCENTER = ALIGN_LEFT | ALIGN_RIGHT,
    ALIGN_HJUSTIFY = 1<<5,

    // combined vertical & horizontal alignments
    ALIGN_TOP_LEFT = ALIGN_TOP | ALIGN_LEFT,
    ALIGN_TOP_RIGHT = ALIGN_TOP | ALIGN_RIGHT,
    ALIGN_TOP_CENTER = ALIGN_TOP | ALIGN_HCENTER,
    ALIGN_BOTTOM_LEFT = ALIGN_BOTTOM | ALIGN_LEFT,
    ALIGN_BOTTOM_RIGHT = ALIGN_BOTTOM | ALIGN_RIGHT,
    ALIGN_BOTTOM_CENTER = ALIGN_BOTTOM | ALIGN_HCENTER,
    ALIGN_CENTER_LEFT = ALIGN_VCENTER | ALIGN_LEFT,
    ALIGN_CENTER_RIGHT = ALIGN_VCENTER | ALIGN_RIGHT,
    ALIGN_CENTER_CENTER = ALIGN_VCENTER | ALIGN_HCENTER,
    ALIGN_CENTER = ALIGN_VCENTER | ALIGN_HCENTER,
    ALIGN_JUSTIFY = ALIGN_VJUSTIFY | ALIGN_HJUSTIFY
  };

  constexpr AlignEnum VAlign(AlignEnum a) {
    return AlignEnum(a & (ALIGN_TOP | ALIGN_BOTTOM | ALIGN_VJUSTIFY)); }

  constexpr AlignEnum HAlign(AlignEnum a) {
    return AlignEnum(a & (ALIGN_LEFT | ALIGN_RIGHT | ALIGN_HJUSTIFY)); }

  enum DrawCmd : uint32_t {
    // control/state commands
    CMD_color,     // <cmd c> (2)
    CMD_lineWidth, // <cmd w> (2)
    CMD_texture,   // <cmd tid> (2)

    // drawing commands
    CMD_line,        // <cmd (x y)x2> (5)
    CMD_triangle,    // <cmd (x y)x3> (7)
    CMD_triangleT,   // <cmd (x y tx ty)x3> (13)
    CMD_triangleC,   // <cmd (x y c)x3> (10)
    CMD_triangleTC,  // <cmd (x y tx ty c)x3> (16)
    CMD_quad,        // <cmd (x y)x4> (9)
    CMD_quadT,       // <cmd (x y tx ty)x4> (17)
    CMD_quadC,       // <cmd (x y c)x4> (13)
    CMD_quadTC,      // <cmd (x y tx ty c)x4> (21)
    CMD_rectangle,   // <cmd (x y)x2> (5)
    CMD_rectangleT,  // <cmd (x y tx ty)x2> (9)
  };

  struct DrawEntry {
    union {
      DrawCmd  cmd;
      float    fval;
      int32_t  ival;
      uint32_t uval;
    };

    DrawEntry(DrawCmd c) : cmd(c) { }
    DrawEntry(float f) : fval(f) { }
    DrawEntry(int32_t i) : ival(i) { }
    DrawEntry(uint32_t u) : uval(u) { }
  };

  struct VertexPT { Vec2 pt, tx; };
  struct VertexPC { Vec2 pt; uint32_t c; };
  struct VertexPTC { Vec2 pt, tx; uint32_t c; };

  class Font;
  class DrawList;
}


class gx::DrawList
{
 public:
  DrawList() { init(); }

  // Low-level data entry
  void clear() { init(); _data.clear(); }
  void reserve(std::size_t n) { _data.reserve(n); }
  [[nodiscard]] std::size_t size() const { return _data.size(); }

  // control/state change
  inline void color(float r, float g, float b, float a = 1.0f);
  inline void color(const Color& c);
  inline void color(uint32_t c);
  inline void lineWidth(float w);
  inline void texture(const Texture& t);

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

  void rectangle(float x, float y, float w, float h) {
    add(CMD_rectangle, x, y, x+w, y+h); }
  void rectangle(float x, float y, float w, float h, Vec2 t0, Vec2 t1) {
    add(CMD_rectangleT, x, y, t0.x, t0.y, x+w, y+h, t1.x, t1.y); }

  // High-level data entry
  void text(const Font& f, float x, float y, AlignEnum align, int spacing,
	    std::string_view text);

  // Data extraction
  [[nodiscard]] const std::vector<DrawEntry>& entries() const { return _data; }

 private:
  std::vector<DrawEntry> _data;
  uint32_t _lastColor;
  float _lastLineWidth;
  int _lastTexID;

  void init() {
    _lastColor = 0xffffffff;
    _lastLineWidth = 1.0f;
    _lastTexID = 0;
  }

  template<typename... Args>
  void add(DrawCmd cmd, const Args&... args) {
    std::initializer_list<DrawEntry> x = {cmd, args...};
    _data.insert(_data.end(), x.begin(), x.end());
  }
};


// **** Inline Implementations ****
void gx::DrawList::color(float r, float g, float b, float a)
{
  uint32_t val = packRGBA8(r, g, b, a);
  if (val != _lastColor) {
    _lastColor = val;
    add(CMD_color, val);
  }
}

void gx::DrawList::color(const Color& c)
{
  uint32_t val = packRGBA8(c.r, c.g, c.b, c.a);
  if (val != _lastColor) {
    _lastColor = val;
    add(CMD_color, val);
  }
}

void gx::DrawList::color(uint32_t c)
{
  if (c != _lastColor) {
    _lastColor = c;
    add(CMD_color, c);
  }
}

void gx::DrawList::lineWidth(float w)
{
  if (w != _lastLineWidth) {
    _lastLineWidth = w;
    add(CMD_lineWidth, w);
  }
}

void gx::DrawList::texture(const Texture& t)
{
  int tid = t.id();
  if (tid != _lastTexID) {
    _lastTexID = tid;
    add(CMD_texture, tid);
  }
}
