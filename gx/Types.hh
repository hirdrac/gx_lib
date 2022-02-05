//
// gx/Types.hh
// Copyright (C) 2022 Richard Bradley
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

  using TextureID = uint32_t;


  // constants
  constexpr Mat4 Mat4Zero     = Matrix4x4Zero<float,ROW_MAJOR>;
  constexpr Mat4 Mat4Identity = Matrix4x4Identity<float,ROW_MAJOR>;


  // enumerations
  enum FilterType {
    FILTER_UNSPECIFIED = 0,
    FILTER_LINEAR,
    FILTER_NEAREST
  };


  // basic types
  struct Rect { float x, y, w, h; };

  struct Vertex2C {
    float x, y; uint32_t c;  // 12 bytes
 
    Vertex2C() = default;
    Vertex2C(Vec2 pos, uint32_t col) : x{pos.x}, y{pos.y}, c{col} { }
    Vertex2C(float px, float py, uint32_t col) : x{px}, y{py}, c{col} { }
  };

  struct Vertex2T { float x, y, s, t; };              // 16
  struct Vertex2TC { float x, y, s, t; uint32_t c; }; // 20

  struct Vertex3C { float x, y, z; uint32_t c; };        // 16
  struct Vertex3T { float x, y, z, s, t; };              // 20
  struct Vertex3TC { float x, y, z, s, t; uint32_t c; }; // 24
}
