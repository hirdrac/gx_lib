//
// gx/Texture.cc
// Copyright (C) 2021 Richard Bradley
//

#include "Texture.hh"
#include "Image.hh"
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

bool gx::Texture::init(Window& win, const Image& img, int levels,
                       FilterType minFilter, FilterType magFilter)
{
  _renderer = win.rendererPtr();
  _texID = _renderer->setTexture(0, img, levels, minFilter, magFilter);
  _width = img.width();
  _height = img.height();
  _levels = levels;
  _minFilter = minFilter;
  _magFilter = magFilter;
  return (_texID != 0);
}

bool gx::Texture::update(const Image& img)
{
  if (!_renderer || _texID == 0) { return false; }
  TextureID id = _renderer->setTexture(
    _texID, img, _levels, _minFilter, _magFilter);
  return (id != 0);
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
