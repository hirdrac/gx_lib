//
// gx/Glyph.hh
// Copyright (C) 2026 Richard Bradley
//
// Rendered glyph data
//

#pragma once
#include "Image.hh"
#include "Types.hh"


struct gx::Glyph
{
  Image bitmap;
  int fontSize;
  float left;        // # pixels at left of image
  float top;         // # pixels above baseline for image top
  float advX, advY;  // x/y cursor advancement
  Vec2 t0, t1;       // texture atlas coords
};
