//
// gx/Glyph.hh
// Copyright (C) 2024 Richard Bradley
//

#pragma once
#include "Types.hh"
#include <memory>


namespace gx {
  struct Glyph;
}

struct gx::Glyph
{
  uint16_t width, height; // image size
  float left;             // # pixels at left of image
  float top;              // # pixels above baseline for image top
  float advX, advY;       // x/y cursor advancement
  const uint8_t* bitmap;
  std::unique_ptr<uint8_t[]> bitmap_copy;

  // texture atlas coords
  Vec2 t0, t1;
};
