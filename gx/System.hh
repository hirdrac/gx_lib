//
// gx/System.hh
// Copyright (C) 2020 Richard Bradley
//
// GLFW init & other misc GLFW/OS function wrappers

#pragma once
#include <string>


namespace gx {
  bool isMainThread();

  bool initGLFW();

  std::string getClipboard();

  void setClipboard(const char* s);
  inline void setClipboard(const std::string& s) { setClipboard(s.c_str()); }
}
