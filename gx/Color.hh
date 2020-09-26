//
// gx/Color.hh
// Copyright (C) 2020 Richard Bradley
//
// Color types & utility functions

#pragma once
#include "types.hh"
#include <algorithm>
#include <cstdint>
#include <endian.h>

namespace gx {
  // types
  using Color = Vec4;

  // color constants
  constexpr Color WHITE  {1.0f, 1.0f, 1.0f, 1.0f};
  constexpr Color BLACK  {0.0f, 0.0f, 0.0f, 1.0f};
  constexpr Color GRAY25 {0.25f, 0.25f, 0.25f, 1.0f};
  constexpr Color GRAY50 {0.5f, 0.5f, 0.5f, 1.0f};
  constexpr Color GRAY75 {0.75f, 0.75f, 0.75f, 1.0f};

  // functions
  static_assert(__BYTE_ORDER == __LITTLE_ENDIAN);

  constexpr uint32_t packRGBA8(float r, float g, float b, float a)
  {
    return uint32_t(std::clamp(r, 0.0f, 1.0f) * 255.0f)
      | (uint32_t(std::clamp(g, 0.0f, 1.0f) * 255.0f) << 8)
      | (uint32_t(std::clamp(b, 0.0f, 1.0f) * 255.0f) << 16)
      | (uint32_t(std::clamp(a, 0.0f, 1.0f) * 255.0f) << 24);
  }

  constexpr uint32_t packRGBA8(const Color& c)
  {
    return packRGBA8(c.r, c.g, c.b, c.a);
  }

  constexpr void unpackRGBA8(uint32_t c, float& r, float& g, float& b, float& a)
  {
    r = float(c & 255) / 255.0f;
    g = float((c >> 8) & 255) / 255.0f;
    b = float((c >> 16) & 255) / 255.0f;
    a = float((c >> 24) & 255) / 255.0f;
  }

  constexpr Color unpackRGBA8(uint32_t c)
  {
    return {float(c & 255) / 255.0f,
	    float((c >> 8) & 255) / 255.0f,
	    float((c >> 16) & 255) / 255.0f,
	    float((c >> 24) & 255) / 255.0f};
  }
}
