//
// DrawLayer.hh
// Copyright (C) 2022 Richard Bradley
//

#pragma once
#include "DrawList.hh"
#include "Color.hh"
#include "Types.hh"


namespace gx {
  struct DrawLayer;

  enum CapabilityEnum {
    BLEND = 1,
    DEPTH_TEST = 2,
    CULL_CW = 4,
    CULL_CCW = 8,
  };
}

struct gx::DrawLayer
{
  DrawList entries;

  // layer specific attributes/flags
  RGBA8 modColor = 0xffffffff;
  int cap = -1;
  Mat4 view, proj;
  bool transformSet = false;
  bool clearDepth = false;
};
