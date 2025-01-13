//
// gx/Clipboard.cc
// Copyright (C) 2025 Richard Bradley
//

#include "Clipboard.hh"
#include "GLFW.hh"
#include "Assert.hh"
#include "ThreadID.hh"


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
