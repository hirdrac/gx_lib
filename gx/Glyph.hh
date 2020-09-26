//
// gx/Glyph.hh
// Copyright (C) 2020 Richard Bradley
//

#pragma once
#include <cstdint>
#include <memory>


namespace gx {
  struct Glyph;
}

struct gx::Glyph
{
  int16_t width, height;  // image size
  float left;             // # pixels at left of image
  float top;              // # pixels above baseline for image top
  float advX, advY;       // x/y cursor advancement
  const uint8_t* bitmap;
  std::unique_ptr<uint8_t[]> bitmap_copy;

  // texture atlas coords
  float tx0, ty0, tx1, ty1;
};
