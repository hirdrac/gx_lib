//
// gx/Color.hh
// Copyright (C) 2021 Richard Bradley
//
// Color types & utility functions
//

#pragma once
#include "Types.hh"
#include <algorithm>

#if __has_include(<endian.h>)
#include <endian.h>
// pack/unpack functions assume Intel byte order
static_assert(__BYTE_ORDER == __LITTLE_ENDIAN);
#endif

namespace gx {
  // types
  using Color = Vec4;
  using RGBA8 = uint32_t;

  // color constants
  constexpr Color WHITE  {1.0f, 1.0f, 1.0f, 1.0f};
  constexpr Color BLACK  {0.0f, 0.0f, 0.0f, 1.0f};
  constexpr Color GRAY25 {0.25f, 0.25f, 0.25f, 1.0f};
  constexpr Color GRAY50 {0.5f, 0.5f, 0.5f, 1.0f};
  constexpr Color GRAY75 {0.75f, 0.75f, 0.75f, 1.0f};

  // functions
  [[nodiscard]] constexpr RGBA8 packRGBA8(float r, float g, float b, float a) {
    return RGBA8(std::clamp(r * 256.0f, 0.0f, 255.0f))
      | (RGBA8(std::clamp(g * 256.0f, 0.0f, 255.0f)) << 8)
      | (RGBA8(std::clamp(b * 256.0f, 0.0f, 255.0f)) << 16)
      | (RGBA8(std::clamp(a * 256.0f, 0.0f, 255.0f)) << 24);
  }

  [[nodiscard]] constexpr RGBA8 packRGBA8(const Color& c) {
    return packRGBA8(c.x, c.y, c.z, c.w);
  }

  [[nodiscard]] constexpr float unpackRed(RGBA8 c) {
    return float(c & 255) / 255.0f; }
  [[nodiscard]] constexpr float unpackGreen(RGBA8 c) {
    return float((c >> 8) & 255) / 255.0f; }
  [[nodiscard]] constexpr float unpackBlue(RGBA8 c) {
    return float((c >> 16) & 255) / 255.0f; }
  [[nodiscard]] constexpr float unpackAlpha(RGBA8 c) {
    return float((c >> 24) & 255) / 255.0f; }

  [[nodiscard]] constexpr Color unpackRGBA8(RGBA8 c) {
    return {unpackRed(c), unpackGreen(c), unpackBlue(c), unpackAlpha(c)};
  }
}
