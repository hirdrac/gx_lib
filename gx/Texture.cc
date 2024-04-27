//
// gx/Texture.cc
// Copyright (C) 2024 Richard Bradley
//

#include "Texture.hh"
#include "Image.hh"
using namespace gx;


bool Texture::init(Renderer& ren, const Image& img, int levels,
                   FilterType minFilter, FilterType magFilter)
{
  cleanup();

  TextureParams params;
  params.levels = levels;
  params.minFilter = minFilter;
  params.magFilter = magFilter;
  params.mipFilter = FILTER_NEAREST;
  params.wrapS = WRAP_CLAMP_TO_EDGE;
  params.wrapT = WRAP_CLAMP_TO_EDGE;

  _texID = ren.newTexture(img, params);
  return (_texID != 0);
}

void Texture::cleanup()
{
  if (_texID != 0) { freeTexture(_texID); }
}
