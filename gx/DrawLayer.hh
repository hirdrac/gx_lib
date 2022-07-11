//
// gx/DrawLayer.hh
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
  Mat4 view{INIT_IDENTITY}, proj{INIT_IDENTITY};
  Vec3 lightPos{0,0,0};
  RGBA8 lightA = 0;  // ambient
  RGBA8 lightD = 0;  // diffuse
  RGBA8 modColor = packRGBA8(WHITE);
  RGBA8 bgColor = 0;
  int32_t cap = -1;
  bool transformSet = false;
  bool clearDepth = false;
  bool useLight = false;

  // helper methods
  void setBGColor(RGBA8 c) {
    bgColor = c; clearDepth = true; }
  void setBGColor(float r, float g, float b) {
    setBGColor(packRGBA8(r,g,b,1.0f)); }
  void setBGColor(const Color& c) {
    setBGColor(packRGBA8(c)); }

  void clear() { entries.clear(); }
};
