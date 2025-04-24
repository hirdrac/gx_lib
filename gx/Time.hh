//
// gx/Time.hh
// Copyright (C) 2025 Richard Bradley
//
// Time related functions
//

#pragma once
#include <chrono>
#include <cstdint>

namespace gx {
  [[nodiscard]] inline int64_t usecTime() {
    using namespace std::chrono;
    return duration_cast<microseconds>(
      steady_clock::now().time_since_epoch()).count();
  }

  [[nodiscard]] inline int64_t secTime() {
    using namespace std::chrono;
    return duration_cast<seconds>(
      steady_clock::now().time_since_epoch()).count();
  }
}
