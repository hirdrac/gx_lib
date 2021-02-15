//
// gx/System.cc
// Copyright (C) 2021 Richard Bradley
//

#include "System.hh"
#include "Logger.hh"
#include <GLFW/glfw3.h>
#include <thread>
#include <cstdlib>
#include <cassert>

namespace {
  std::thread::id mainThreadID = std::this_thread::get_id();
    // limitation of GLFW, most API calls must be from main thread

  void errorCB(int error, const char* txt)
  {
    GX_LOG_ERROR("GLFW ERROR(", error, "): ", txt);
  }
}

bool gx::isMainThread()
{
  return std::this_thread::get_id() == mainThreadID;
}

bool gx::initGLFW()
{
  static bool init = false;
  if (init) { return true; }

  init = true;
  glfwSetErrorCallback(errorCB);
  //glfwInitHint(GLFW_JOYSTICK_HAT_BUTTONS, GLFW_FALSE);
  if (!glfwInit()) {
    GX_LOG_ERROR("glfwInit() failed");
    return false;
  }

  std::atexit(glfwTerminate);
  return true;
}

std::string gx::getClipboard()
{
  assert(isMainThread());

  initGLFW();
  const char* txt = glfwGetClipboardString(nullptr);
  return (txt == nullptr) ? std::string() : txt;
}

void gx::setClipboard(const char* s)
{
  assert(isMainThread());

  initGLFW();
  glfwSetClipboardString(nullptr, s);
}
