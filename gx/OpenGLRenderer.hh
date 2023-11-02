//
// gx/OpenGLRenderer.hh
// Copyright (C) 2023 Richard Bradley
//

#pragma once
#include "Renderer.hh"
#include <memory>


namespace gx {
  [[nodiscard]] std::unique_ptr<Renderer> makeOpenGLRenderer(GLFWwindow* win);
}
