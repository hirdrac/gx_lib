//
// gx/Texture.cc
// Copyright (C) 2024 Richard Bradley
//

#include "Texture.hh"
#include "Image.hh"
#include "Renderer.hh"
using namespace gx;


bool Texture::init(Renderer& ren, const Image& img, int levels,
                   FilterType minFilter, FilterType magFilter)
{
  TextureParams params;
  params.minFilter = minFilter;
  params.magFilter = magFilter;
  params.mipFilter = FILTER_NEAREST;

  _texID = ren.setTexture(0, img, levels, params);
  _width = img.width();
  _height = img.height();
  _levels = levels;
  return (_texID != 0);
}

bool Texture::update(const Image& img)
{
  return updateTexture(_texID, img, _levels, {});
}

void Texture::cleanup()
{
  if (_texID != 0) { freeTexture(_texID); }
}
