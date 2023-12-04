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
  int32_t cap = -1;

  // helper methods
  void clear() { entries.clear(); }
};
