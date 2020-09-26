//
// gx/Texture.cc
// Copyright (C) 2020 Richard Bradley
//

#include "Texture.hh"
#include "Window.hh"
#include "Renderer.hh"


gx::Texture::Texture() = default;

gx::Texture& gx::Texture::operator=(Texture&& t) noexcept
{
  cleanup();
  _renderer = t._renderer;
  t._renderer.reset();
  _texID = std::exchange(t._texID,0);
  return *this;
}

bool gx::Texture::init(Window& win, const Image& img)
{
  _renderer = win._renderer;
  _texID = _renderer->addTexture(img);
  return (_texID != -1);
}

bool gx::Texture::update(const Image& img)
{
  if (!_renderer) { return false; }
  return _renderer->updateTexture(_texID, img);
}

void gx::Texture::cleanup()
{
  if (_renderer) {
    _renderer->freeTexture(_texID);

    // **** unnecessary ****
    //_renderer.reset();
    //_texID = 0
  }
}
