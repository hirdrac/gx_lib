//
// gx/types.hh
// Copyright (C) 2020 Richard Bradley
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


  // constants
  constexpr Mat4 Mat4Zero     = Matrix4x4Zero<float,ROW_MAJOR>;
  constexpr Mat4 Mat4Identity = Matrix4x4Identity<float,ROW_MAJOR>;
}
