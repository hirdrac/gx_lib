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
  using RGBA8 = uint32_t;

  // color constants
  constexpr Color WHITE   {1.0f, 1.0f, 1.0f, 1.0f};
  constexpr Color BLACK   {0.0f, 0.0f, 0.0f, 1.0f};
  constexpr Color GRAY25  {0.25f, 0.25f, 0.25f, 1.0f};
  constexpr Color GRAY50  {0.5f, 0.5f, 0.5f, 1.0f};
  constexpr Color GRAY75  {0.75f, 0.75f, 0.75f, 1.0f};

  constexpr Color RED     {1.0f, 0.0f, 0.0f, 1.0f};
  constexpr Color GREEN   {0.0f, 1.0f, 0.0f, 1.0f};
  constexpr Color BLUE    {0.0f, 0.0f, 1.0f, 1.0f};
  constexpr Color CYAN    {0.0f, 1.0f, 1.0f, 1.0f};
  constexpr Color YELLOW  {1.0f, 1.0f, 0.0f, 1.0f};
  constexpr Color MAGENTA {1.0f, 0.0f, 1.0f, 1.0f};

  // functions
  [[nodiscard]] constexpr RGBA8 packRGBA8i(
    uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return RGBA8(r) | (RGBA8(g) << 8) | (RGBA8(b) << 16) | (RGBA8(a) << 24);
  }

  [[nodiscard]] constexpr RGBA8 packRGBA8(float r, float g, float b, float a) {
    return RGBA8(std::clamp(r * 256.0f, 0.0f, 255.0f))
      | (RGBA8(std::clamp(g * 256.0f, 0.0f, 255.0f)) << 8)
      | (RGBA8(std::clamp(b * 256.0f, 0.0f, 255.0f)) << 16)
      | (RGBA8(std::clamp(a * 256.0f, 0.0f, 255.0f)) << 24);
  }

  [[nodiscard]] constexpr RGBA8 packRGBA8(const Color& c) {
    return packRGBA8(c.x, c.y, c.z, c.w);
  }

  [[nodiscard]] constexpr uint8_t unpackRedInt(RGBA8 c) {
    return uint8_t(c & 255); }
  [[nodiscard]] constexpr float unpackRedFloat(RGBA8 c) {
    return float(unpackRedInt(c)) / 255.0f; }

  [[nodiscard]] constexpr uint8_t unpackGreenInt(RGBA8 c) {
    return uint8_t((c >> 8) & 255); }
  [[nodiscard]] constexpr float unpackGreenFloat(RGBA8 c) {
    return float(unpackGreenInt(c)) / 255.0f; }

  [[nodiscard]] constexpr uint8_t unpackBlueInt(RGBA8 c) {
    return uint8_t((c >> 16) & 255); }
  [[nodiscard]] constexpr float unpackBlueFloat(RGBA8 c) {
    return float(unpackBlueInt(c)) / 255.0f; }

  [[nodiscard]] constexpr uint8_t unpackAlphaInt(RGBA8 c) {
    return uint8_t((c >> 24) & 255); }
  [[nodiscard]] constexpr float unpackAlphaFloat(RGBA8 c) {
    return float(unpackAlphaInt(c)) / 255.0f; }

  [[nodiscard]] constexpr Color unpackRGBA8(RGBA8 c) {
    return {unpackRedFloat(c), unpackGreenFloat(c),
      unpackBlueFloat(c), unpackAlphaFloat(c)};
  }
}
