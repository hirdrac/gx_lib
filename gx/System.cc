//
// gx/System.cc
// Copyright (C) 2024 Richard Bradley
//

#include "System.hh"
#include "Logger.hh"
#include "Assert.hh"
#include "StringUtil.hh"
#include "ThreadID.hh"
#include <GLFW/glfw3.h>
#include <string_view>
#include <cstdlib>

namespace {
  void errorCB(int error, const char* txt)
  {
    GX_LOG_ERROR("GLFW ERROR(", error, "): ", txt);
  }

  bool lib_init = false;

  void shutdown()
  {
    glfwTerminate();
    lib_init = false;
  }

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

  std::atexit(shutdown);
  return true;
}

bool gx::glfwInitStatus()
{
  return lib_init;
}

std::string gx::getClipboardFull()
{
  GX_ASSERT(isMainThread());

  initGLFW();
  const char* txt = glfwGetClipboardString(nullptr);
  return (txt == nullptr) ? std::string{} : txt;
}

std::string gx::getClipboardFirstLine()
{
  GX_ASSERT(isMainThread());

  initGLFW();
  const char* txt = glfwGetClipboardString(nullptr);
  if (txt == nullptr) { return {}; }

  const std::string_view sv{txt};
  return std::string{sv.substr(0, sv.find('\n'))};
}

void gx::setClipboard(const char* s)
{
  GX_ASSERT(isMainThread());

  initGLFW();
  glfwSetClipboardString(nullptr, s);
}
