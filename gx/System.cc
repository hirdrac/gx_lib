//
// gx/System.cc
// Copyright (C) 2020 Richard Bradley
//

#include "System.hh"
#include "Logger.hh"
#include <GLFW/glfw3.h>
#include <thread>
#include <cstdlib>
#include <cassert>

namespace {
  std::thread::id mainThreadID = std::this_thread::get_id();

  void errorCB(int error, const char* txt)
  {
    LOG_ERROR("GLFW ERROR(", error, "): ", txt);
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
  if (!glfwInit()) {
    LOG_ERROR("glfwInit() failed");
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
