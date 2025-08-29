//
// gx/GLFW.cc
// Copyright (C) 2025 Richard Bradley
//

#include "GLFW.hh"
#include "StringUtil.hh"
#include "Logger.hh"
#include <cstdlib>

namespace {
  [[nodiscard]] std::string_view errorStr(int error)
  {
    #define ERROR_CASE(x) case GLFW_##x: return #x
    switch (error) {
      ERROR_CASE(NO_ERROR);
      ERROR_CASE(NOT_INITIALIZED);
      ERROR_CASE(NO_CURRENT_CONTEXT);
      ERROR_CASE(INVALID_ENUM);
      ERROR_CASE(INVALID_VALUE);
      ERROR_CASE(OUT_OF_MEMORY);
      ERROR_CASE(API_UNAVAILABLE);
      ERROR_CASE(VERSION_UNAVAILABLE);
      ERROR_CASE(PLATFORM_ERROR);
      ERROR_CASE(FORMAT_UNAVAILABLE);
      ERROR_CASE(NO_WINDOW_CONTEXT);
      ERROR_CASE(CURSOR_UNAVAILABLE);
      ERROR_CASE(FEATURE_UNAVAILABLE);
      ERROR_CASE(FEATURE_UNIMPLEMENTED);
      ERROR_CASE(PLATFORM_UNAVAILABLE);
      default: return "unknown";
    }
    #undef ERROR_CASE
  }

  void errorCB(int error, const char* txt)
  {
    GX_LOG_ERROR("GLFW ERROR(", errorStr(error), " 0x", gx::formatHexUC(error), "): ", txt);
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
