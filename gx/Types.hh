//
// gx/Types.hh
// Copyright (C) 2025 Richard Bradley
//

#pragma once
#include "Vector3D.hh"
#include "Matrix3D.hh"
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
  struct TextFormat;
  class Window;


  // basic types
  struct Rect {
    float x, y, w, h;

    Rect() = default;
    constexpr Rect(float rw, float rh)
      : x{0}, y{0}, w{rw}, h{rh} { }
    constexpr Rect(float rx, float ry, float rw, float rh)
      : x{rx}, y{ry}, w{rw}, h{rh} { }
  };

  struct Value {
    union {
      int32_t  ival;
      uint32_t uval;
      float    fval;
    };

    Value(int32_t i) : ival{i} { }
    Value(uint32_t u) : uval{u} { }
    Value(float f) : fval{f} { }
  };
  static_assert(sizeof(Value) == 4);
}
