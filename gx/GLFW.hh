//
// gx/GLFW.hh
// Copyright (C) 2025 Richard Bradley
//
// GLFW library init & support functions
//

#pragma once
#include <GLFW/glfw3.h>

namespace gx {
  bool initGLFW();
    // initialized library & setup error logging

  [[nodiscard]] bool glfwInitStatus();
    // returns true if GLFW is initialized

  [[nodiscard]] constexpr int glfwBool(bool val) {
    return val ? GLFW_TRUE : GLFW_FALSE; }
}
