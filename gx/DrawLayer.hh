//
// gx/DrawLayer.hh
// Copyright (C) 2023 Richard Bradley
//

// TODO: remove DrawLayer

#pragma once
#include "DrawList.hh"


namespace gx {
  struct DrawLayer;
}

struct gx::DrawLayer
{
  DrawList entries;

  // helper methods
  void clear() { entries.clear(); }
};
