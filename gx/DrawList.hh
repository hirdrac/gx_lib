//
// gx/DrawList.hh
// Copyright (C) 2020 Richard Bradley
//

// TODO - vertical/horizontal color gradiant
// TODO - clipping region
// TODO - continuous lines [lineX <vertex count> <v1> <v2> ...]

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
    ALIGN_LEFT = 1<<2,
    ALIGN_RIGHT = 1<<3,
    ALIGN_HCENTER = ALIGN_LEFT | ALIGN_RIGHT,
    ALIGN_HJUSTIFY = 1<<4,

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

  enum DrawCmd : uint32_t {
    // control/state commands
    CMD_color,     // <color c>(2)
    CMD_lineWidth, // <lineWidth w>(2)

    // drawing commands
    CMD_line,      // <line x0 y0 x1 y1>(5)
    CMD_triangle,  // <triangle x0 y0 x1 y1 x2 y2>(7)
    CMD_rectangle, // <rectangle x y w h>(5)
    CMD_image,     // <image tex x y w h tx0 ty0 tx1 ty1>(10)
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

  class Font;
  class DrawList;
}


class gx::DrawList
{
 public:
  // Low-level data entry
  void clear() { _data.clear(); }
  void reserve(std::size_t n) { _data.reserve(n); }
  [[nodiscard]] std::size_t size() const { return _data.size(); }

  void color(float r, float g, float b, float a = 1.0f) {
    add(CMD_color, packRGBA8(r, g, b, a)); }
  void color(const Color& c) {
    add(CMD_color, packRGBA8(c.r, c.g, c.b, c.a)); }
  void color(uint32_t c) {
    add(CMD_color, c); }

  void lineWidth(float w) { add(CMD_lineWidth, w); }

  void line(Vec2 p0, Vec2 p1) {
    add(CMD_line, p0.x, p0.y, p1.x, p1.y); }
  void line(float x0, float y0, float x1, float y1) {
    add(CMD_line, x0, y0, x1, y1); }

  void triangle(Vec2 p0, Vec2 p1, Vec2 p2) {
    add(CMD_triangle, p0.x, p0.y, p1.x, p1.y, p2.x, p2.y); }
  void rectangle(float x, float y, float w, float h) {
    add(CMD_rectangle, x, y, x+w, y+h); }

  void image(int32_t tex, float x, float y, float w, float h,
	     float tx0, float ty0, float tx1, float ty1) {
    add(CMD_image, tex, x, y, x+w, y+h, tx0, ty0, tx1, ty1); }
  void image(const Texture& tex, float x, float y, float w, float h,
	     float tx0, float ty0, float tx1, float ty1) {
    image(tex.id(), x, y, w, h, tx0, ty0, tx1, ty1); }

  // High-level data entry
  void text(const Font& f, float x, float y, AlignEnum align, int spacing,
	    std::string_view text);

  // Data extraction
  [[nodiscard]] const std::vector<DrawEntry>& entries() const { return _data; }

 private:
  std::vector<DrawEntry> _data;

  template<typename... Args>
  void add(DrawCmd cmd, const Args&... args) {
    std::initializer_list<DrawEntry> x = {cmd, args...};
    _data.insert(_data.end(), x.begin(), x.end());
  }
};
