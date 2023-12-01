//
// gx/DrawLayer.hh
// Copyright (C) 2023 Richard Bradley
//

// TODO: move attributes to DrawEntry commands
// TODO: add flag for 'reset state' (default true)

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
  Mat4 view{INIT_IDENTITY}, proj{INIT_IDENTITY};
  RGBA8 modColor = packRGBA8(WHITE);
  int32_t cap = -1;
  bool transformSet = false;

  // helper methods
  void clear() { entries.clear(); }
};
