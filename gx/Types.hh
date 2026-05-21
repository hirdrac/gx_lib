//
// gx/Types.hh
// Copyright (C) 2026 Richard Bradley
//

#pragma once
#include "Vector3D.hh"
#include "Matrix3D.hh"
#include "Align.hh"
#include <cstdint>

namespace gx {
  // type aliases
  using Vec2 = Vector2<float>;
  using Vec3 = Vector3<float>;
  using Vec4 = Vector4<float>;
  using Mat4 = Matrix4x4<float,ROW_MAJOR>;


  // forward declare major types
  class Camera;
  class DrawContext;
  class DrawList;
  struct EventState;
  class Font;
  struct Glyph;
  class Image;
  class Renderer;
  struct Style;
  struct TextFormat;
  class TextMetaState;
  class Window;


  // basic types
  struct Rect {
    float x, y, w, h;

    [[nodiscard]] bool operator==(const Rect&) const = default;

    [[nodiscard]] explicit operator bool() const { return w > 0 && y > 0; }
      // valid Rect must have width/height greater than zero

    [[nodiscard]] constexpr bool contains(Vec2 pt) const {
      return (pt.x >= x) && (pt.x < (x+w)) && (pt.y >= y) && (pt.y < (y+h)); }

    [[nodiscard]] float alignX(Align align) const {
      switch (hAlign(align)) {
        case Align::left:      return x;
        case Align::right:     return x + w;
        default: /* hcenter */ return x + std::floor(w * .5f);
      }
    }

    [[nodiscard]] float alignX(Align align, float width) const {
      switch (hAlign(align)) {
        case Align::left:      return x;
        case Align::right:     return x + w - width;
        default: /* hcenter */ return x + std::floor((w - width) * .5f);
      }
    }

    [[nodiscard]] float alignY(Align align) const {
      switch (vAlign(align)) {
        case Align::top:       return y;
        case Align::bottom:    return y + h;
        default: /* vcenter */ return y + std::floor(h * .5f);
      }
    }

    [[nodiscard]] float alignY(Align align, float height) const {
      switch (vAlign(align)) {
        case Align::top:       return y;
        case Align::bottom:    return y + h - height;
        default: /* vcenter */ return y + std::floor((h - height) * .5f);
      }
    }

    [[nodiscard]] Vec2 alignPt(Align align) const {
      return {alignX(align), alignY(align)}; }

    [[nodiscard]] Rect alignRect(Align align, float rw, float rh) const {
      return {alignX(align, rw), alignY(align, rh), rw, rh}; }

    [[nodiscard]] Rect clip(const Rect& r) const {
      const float x0 = std::max(x, r.x);
      const float x1 = std::min(x + w, r.x + r.w);
      const float y0 = std::max(y, r.y);
      const float y1 = std::min(y + h, r.y + r.h);
      return {x0, y0, x1 - x0, y1 - y0};
    }
  };
}
