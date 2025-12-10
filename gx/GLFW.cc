//
// gx/GLFW.cc
// Copyright (C) 2025 Richard Bradley
//

#include "GLFW.hh"
#include "StringUtil.hh"
#include "Logger.hh"
#include "Init.hh"
#include <cstdlib>
using namespace gx;


Platform gx::initPlatform = Platform::unspecified;

namespace {
  [[nodiscard]] std::string_view platformStr(int platform)
  {
    #define PLATFORM_CASE(x) case GLFW_PLATFORM_##x: return #x;
    switch (platform) {
      PLATFORM_CASE(WIN32);
      PLATFORM_CASE(COCOA);
      PLATFORM_CASE(WAYLAND);
      PLATFORM_CASE(X11);
      default: return "unknown";
    }
    #undef PLATFORM_CASE
  }

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
    GX_LOG_ERROR("GLFW ERROR(", errorStr(error), " 0x",
                 formatHexUC(uint64_t(error)), "): ", txt);
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
    return concat(major, '.', minor, '.', revision);
  }

  bool tryPlatform(Platform p)
  {
    int glfw_platform;
    switch (p) {
      case Platform::x11:
        glfw_platform = GLFW_PLATFORM_X11; break;
      case Platform::wayland:
        glfw_platform = GLFW_PLATFORM_WAYLAND; break;
      case Platform::win32:
        glfw_platform = GLFW_PLATFORM_WIN32; break;
      case Platform::cocoa:
        glfw_platform = GLFW_PLATFORM_COCOA; break;
      default:
        return false;
    }

    if (!glfwBool(glfwPlatformSupported(glfw_platform))) { return false; }

    glfwInitHint(GLFW_PLATFORM, glfw_platform);
    return true;
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

  tryPlatform(initPlatform);

#if 0
  if (glfwBool(glfwPlatformSupported(GLFW_PLATFORM_WAYLAND))
      && glfwBool(glfwPlatformSupported(GLFW_PLATFORM_X11))) {
    // force X11 instead of wayland because of wayland issues
    // TODO: add option to force wayland in this case
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
  }
#endif

  if (!glfwInit()) {
    GX_LOG_ERROR("glfwInit() failed");
    return false;
  }

  GX_LOG_INFO("GLFW platform: ", platformStr(glfwGetPlatform()));

  //std::atexit(shutdown);
  return true;
}

bool gx::glfwInitStatus()
{
  return lib_init;
}
