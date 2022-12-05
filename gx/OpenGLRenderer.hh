//
// gx/OpenGLRenderer.hh
// Copyright (C) 2022 Richard Bradley
//

#pragma once
#include "Renderer.hh"
#include <memory>


namespace gx {
  std::unique_ptr<Renderer> makeOpenGLRenderer(GLFWwindow* win);
}
