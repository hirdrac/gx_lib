//
// gx/ThreadID.hh
// Copyright (C) 2024 Richard Bradley
//

#pragma once
#include <cstdint>

namespace gx {
  // globals
  extern const uint64_t mainThreadID;

  // functions
  [[nodiscard]] uint64_t getThreadID();
  [[nodiscard]] inline bool isMainThread() {
    return mainThreadID == getThreadID(); }
}
