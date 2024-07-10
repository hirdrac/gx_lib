//
// gx/TextFormat.cc
// Copyright (C) 2024 Richard Bradley
//

#include "TextFormat.hh"
#include "Font.hh"
#include "Assert.hh"
using namespace gx;


float TextFormat::calcLength(std::string_view text) const
{
  GX_ASSERT(font != nullptr);

  // TODO: use length of advX vector to adjust length calc
  // TODO: add width/height calc for multi-line text
  return font->calcLength(text, glyphSpacing);
}

std::string_view TextFormat::fitText(
  std::string_view text, float maxLength) const
{
  GX_ASSERT(font != nullptr);

  return font->fitText(text, glyphSpacing, maxLength);
}
