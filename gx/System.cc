//
// gx/System.cc
// Copyright (C) 2022 Richard Bradley
//

#include "System.hh"
#include "Logger.hh"
#include <GLFW/glfw3.h>
#include <thread>
#include <string_view>
#include <cstdlib>
#include <cassert>

namespace {
  const std::thread::id mainThreadID = std::this_thread::get_id();
    // limitation of GLFW, most API calls must be from main thread

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
}

bool gx::isMainThread()
{
  return std::this_thread::get_id() == mainThreadID;
}

bool gx::initGLFW()
{
  if (lib_init) { return true; }

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
  assert(isMainThread());

  initGLFW();
  const char* txt = glfwGetClipboardString(nullptr);
  return (txt == nullptr) ? std::string() : txt;
}

std::string gx::getClipboardFirstLine()
{
  assert(isMainThread());

  initGLFW();
  const char* txt = glfwGetClipboardString(nullptr);
  if (txt == nullptr) { return std::string(); }

  std::string_view sv{txt};
  return std::string{sv.substr(0, sv.find('\n'))};
}

void gx::setClipboard(const char* s)
{
  assert(isMainThread());

  initGLFW();
  glfwSetClipboardString(nullptr, s);
}
