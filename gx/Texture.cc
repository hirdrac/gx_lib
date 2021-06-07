//
// gx/Texture.cc
// Copyright (C) 2021 Richard Bradley
//

#include "Texture.hh"
#include "Image.hh"
#include "Renderer.hh"


gx::Texture::Texture() = default;

gx::Texture& gx::Texture::operator=(Texture&& t) noexcept
{
  cleanup();
  _texID = std::exchange(t._texID,0);
  return *this;
}

bool gx::Texture::init(Renderer& ren, const Image& img, int levels,
                       FilterType minFilter, FilterType magFilter)
{
  _texID = ren.setTexture(0, img, levels, minFilter, magFilter);
  _width = img.width();
  _height = img.height();
  _levels = levels;
  _minFilter = minFilter;
  _magFilter = magFilter;
  registerTextureOwner(_texID, &ren);
  return (_texID != 0);
}

bool gx::Texture::update(const Image& img)
{
  return updateTexture(_texID, img, _levels, _minFilter, _magFilter);
}

void gx::Texture::cleanup()
{
  freeTexture(_texID);
}
