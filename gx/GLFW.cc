//
// gx/GLFW.cc
// Copyright (C) 2025 Richard Bradley
//

#include "GLFW.hh"
#include "StringUtil.hh"
#include "Logger.hh"
#include <cstdlib>

namespace {
  void errorCB(int error, const char* txt)
  {
    GX_LOG_ERROR("GLFW ERROR(", error, "): ", txt);
  }

  bool lib_init = false;

#if 0
  void shutdown()
  {
    glfwTerminate();
    lib_init = false;
  }
#endif

  [[nodiscard]] std::string libVersionStr()
  {
    int major = 0, minor = 0, revision = 0;
    glfwGetVersion(&major, &minor, &revision);
    return gx::concat(major, '.', minor, '.', revision);
  }
}

bool gx::initGLFW()
{
  if (lib_init) { return true; }

  GX_LOG_INFO("GLFW compiled version: ", GLFW_VERSION_MAJOR, ".",
              GLFW_VERSION_MINOR, ".", GLFW_VERSION_REVISION);
  GX_LOG_INFO("GLFW library version: ", libVersionStr());

  lib_init = true;
  glfwSetErrorCallback(errorCB);
  //glfwInitHint(GLFW_JOYSTICK_HAT_BUTTONS, GLFW_FALSE);
  if (!glfwInit()) {
    GX_LOG_ERROR("glfwInit() failed");
    return false;
  }

  //std::atexit(shutdown);
  return true;
}

bool gx::glfwInitStatus()
{
  return lib_init;
}
