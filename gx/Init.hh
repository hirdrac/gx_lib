//
// gx/Init.hh
// Copyright (C) 2025 Richard Bradley
//
// Flags/values to set before using gx_lib
//

#pragma once

namespace gx {
  enum class Platform { unspecified, x11, wayland, win32, cocoa };

  extern Platform initPlatform;
}
