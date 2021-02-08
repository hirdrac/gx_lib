//
// gx/RendererPtr.hh
// Copyright (C) 2021 Richard Bradley
//

#pragma once
#include <memory>


namespace gx {
  class Renderer;

  using RendererPtr = std::shared_ptr<Renderer>;
}
