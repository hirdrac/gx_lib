//
// gx/Normal.hh
// Copyright (C) 2025 Richard Bradley
//

#pragma once
#include "Types.hh"
#include <algorithm>

namespace gx {
  // functions to convert normal vectors into/from 32-bit unsigned ints
  // (10 bits for each component)
  [[nodiscard]] constexpr uint32_t packNormal(float x, float y, float z) {
    // encoded value is (0,1022) so values -1,0,1 can be exactly encoded/decoded
    return uint32_t(std::clamp(x + 1.0f, 0.0f, 2.0f) * 511.0f + .5f)
      | (uint32_t(std::clamp(y + 1.0f, 0.0f, 2.0f) * 511.0f + .5f) << 10)
      | (uint32_t(std::clamp(z + 1.0f, 0.0f, 2.0f) * 511.0f + .5f) << 20);
  }

  template<class T>
  [[nodiscard]] constexpr uint32_t packNormal(const T& n) {
    return packNormal(n[0], n[1], n[2]);
  }

  [[nodiscard]] constexpr Vec3 unpackNormal(uint32_t n) {
    return {
      float(n & 1023) / 511.0f - 1.0f,
      float((n >> 10) & 1023) / 511.0f - 1.0f,
      float((n >> 20) & 1023) / 511.0f - 1.0f
    };
  }
}
