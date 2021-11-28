//
// gx/System.hh
// Copyright (C) 2021 Richard Bradley
//
// GLFW init & other misc GLFW/OS function wrappers
//

#pragma once
#include <string>


namespace gx {
  [[nodiscard]] bool isMainThread();

  bool initGLFW();
    // initialized library & setup error logging

  [[nodiscard]] bool glfwInitStatus();
    // returns true if GLFW is initialized

  std::string getClipboardFull();
  std::string getClipboardFirstLine();

  void setClipboard(const char* s);
  inline void setClipboard(const std::string& s) { setClipboard(s.c_str()); }
}
