//
// gx/Texture.cc
// Copyright (C) 2022 Richard Bradley
//

#include "Texture.hh"
#include "Image.hh"
#include "Renderer.hh"
using namespace gx;


bool Texture::init(Renderer& ren, const Image& img, int levels,
                   FilterType minFilter, FilterType magFilter)
{
  _texID = ren.setTexture(0, img, levels, minFilter, magFilter);
  _width = img.width();
  _height = img.height();
  _levels = levels;
  _minFilter = minFilter;
  _magFilter = magFilter;
  return (_texID != 0);
}

bool Texture::update(const Image& img)
{
  return updateTexture(_texID, img, _levels, _minFilter, _magFilter);
}

void Texture::cleanup()
{
  if (_texID != 0) { freeTexture(_texID); }
}
