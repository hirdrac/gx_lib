//
// gx/Normal.hh
// Copyright (C) 2023 Richard Bradley
//

#pragma once
#include "Types.hh"
#include <algorithm>

namespace gx {
  // functions to convert normal vectors into/from 32-bit unsigned ints
  // (10 bits for each component)
  constexpr uint32_t packNormal(const Vec3& n) {
    return uint32_t(int32_t(std::clamp(n.x, -1.0f, 1.0f) * 511.0f) + 511)
      | uint32_t(int32_t(std::clamp(n.y, -1.0f, 1.0f) * 511.0f) + 511) << 10
      | uint32_t(int32_t(std::clamp(n.z, -1.0f, 1.0f) * 511.0f) + 511) << 20;
  }

  constexpr Vec3 unpackNormal(uint32_t n) {
    return {
      float(int32_t(n & 0x3ff) - 511) / 511.0f,
      float(int32_t((n>>10) & 0x3ff) - 511) / 511.0f,
      float(int32_t((n>>20) & 0x3ff) - 511) / 511.0f};
  }
}
