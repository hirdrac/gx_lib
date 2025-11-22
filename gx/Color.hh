//
// gx/Color.hh
// Copyright (C) 2025 Richard Bradley
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
  using RGBA8 = uint32_t;  // red/green/blue/alpha 8 bits per channel

  // color constants
  constexpr Vec3 WHITE   {1.0f, 1.0f, 1.0f};
  constexpr Vec3 BLACK   {0.0f, 0.0f, 0.0f};
  constexpr Vec3 GRAY25  {0.25f, 0.25f, 0.25f};
  constexpr Vec3 GRAY50  {0.5f, 0.5f, 0.5f};
  constexpr Vec3 GRAY75  {0.75f, 0.75f, 0.75f};

  constexpr Vec3 RED     {1.0f, 0.0f, 0.0f};
  constexpr Vec3 GREEN   {0.0f, 1.0f, 0.0f};
  constexpr Vec3 BLUE    {0.0f, 0.0f, 1.0f};
  constexpr Vec3 CYAN    {0.0f, 1.0f, 1.0f};
  constexpr Vec3 YELLOW  {1.0f, 1.0f, 0.0f};
  constexpr Vec3 MAGENTA {1.0f, 0.0f, 1.0f};

  // RGBA8 functions
  [[nodiscard]] constexpr RGBA8 packRGBA8i(
    uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return RGBA8(r) | (RGBA8(g) << 8) | (RGBA8(b) << 16) | (RGBA8(a) << 24);
  }

  [[nodiscard]] constexpr RGBA8 packRGBA8(
    float r, float g, float b, float a = 1.0f) {
    return packRGBA8i(
      uint8_t(std::clamp(r, 0.0f, 1.0f) * 255.0f + .5f),
      uint8_t(std::clamp(g, 0.0f, 1.0f) * 255.0f + .5f),
      uint8_t(std::clamp(b, 0.0f, 1.0f) * 255.0f + .5f),
      uint8_t(std::clamp(a, 0.0f, 1.0f) * 255.0f + .5f));
  }

  [[nodiscard]] constexpr RGBA8 packRGBA8(const Vec3& c, float a = 1.0f) {
    return packRGBA8(c.x, c.y, c.z, a); }
  [[nodiscard]] constexpr RGBA8 packRGBA8(const Vec4& c) {
    return packRGBA8(c.x, c.y, c.z, c.w); }

  [[nodiscard]] constexpr float unpackRGBA8Red(RGBA8 c) {
    return float(c & 255) / 255.0f; }
  [[nodiscard]] constexpr float unpackRGBA8Green(RGBA8 c) {
    return float((c >> 8) & 255) / 255.0f; }
  [[nodiscard]] constexpr float unpackRGBA8Blue(RGBA8 c) {
    return float((c >> 16) & 255) / 255.0f; }
  [[nodiscard]] constexpr float unpackRGBA8Alpha(RGBA8 c) {
    return float(c >> 24) / 255.0f; }

  [[nodiscard]] constexpr Vec4 unpackRGBA8(RGBA8 c) {
    return {unpackRGBA8Red(c), unpackRGBA8Green(c),
      unpackRGBA8Blue(c), unpackRGBA8Alpha(c)};
  }
}
