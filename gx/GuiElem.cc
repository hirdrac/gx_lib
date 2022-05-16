//
// gx/GuiElem.cc
// Copyright (C) 2022 Richard Bradley
//

#include "GuiElem.hh"
#include "Renderer.hh"
using namespace gx;


// **** GuiTexture class ****
void GuiTexture::cleanup()
{
  if (_tid != 0) { freeTexture(_tid); }
}
