//
// gx/Types.hh
// Copyright (C) 2026 Richard Bradley
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
  class DrawContext2D;
  class DrawContext3D;
  class DrawList;
  struct EventState;
  class Font;
  struct Glyph;
  class IDRegionList;
  class Image;
  class RandomSequence;
  struct Rect;
  class Renderer;
  struct Style;
  struct TextFormat;
  class TextMetaState;
  class Window;
}
