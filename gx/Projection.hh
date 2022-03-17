//
// gx/Projection.hh
// Copyright (C) 2022 Richard Bradley
//

#pragma once
#include "Types.hh"


namespace gx {
  // functions
  constexpr Mat4 orthoProjection(float width, float height);
    // simple orthogonal projection to work in screen coordinates
    //  x:[0 width] => x:[-1 1]
    //  y:[0 height]   y:[-1 1]
    // origin in upper left corner
}


// **** Inline Implementations ****
constexpr gx::Mat4 gx::orthoProjection(float width, float height)
{
  return {
    2.0f / width, 0, 0, 0,
    0, -2.0f / height, 0, 0,
    // negative value flips vertical direction for OpenGL
    // FIXME: handle vertical flip inside OpenGLRenderer in the future
    0, 0, 1, 0,
    -1, 1, 0, 1};
}
