//
// gx/TextFormat.cc
// Copyright (C) 2024 Richard Bradley
//

#include "TextFormat.hh"
#include "Font.hh"
#include "Unicode.hh"
#include "Assert.hh"
using namespace gx;


float TextFormat::calcLength(std::string_view text) const
{
  GX_ASSERT(font != nullptr);

  // TODO: use length of advX vector to adjust length calc
  // TODO: add width/height calc for multi-line text

  float max_len = 0, len = -glyphSpacing;
  for (UTF8Iterator itr{text}; !itr.done(); itr.next()) {
    int32_t ch = itr.get();
    if (ch == '\t') {
      ch = ' '; // tab logic should be handled outside of this function
    } else if (ch == '\n') {
      max_len = std::max(max_len, len);
      len = -glyphSpacing;
    }

    const Glyph* g = font->findGlyph(ch);
    if (!g) {
      g = font->findGlyph(font->unknownCode());
      GX_ASSERT(g != nullptr);
    }

    len += g->advX + glyphSpacing;
  }

  return std::max(max_len, len);
}

std::string_view TextFormat::fitText(
  std::string_view text, float maxLength) const
{
  GX_ASSERT(font != nullptr);

  float len = -glyphSpacing;
  for (UTF8Iterator itr{text}; !itr.done(); itr.next()) {
    int32_t ch = itr.get();
    if (ch == '\t') {
      ch = ' ';
    } else if (ch == '\n') {
      // stop at first linebreak
      return text.substr(0, itr.pos());
    }

    const Glyph* g = font->findGlyph(ch);
    if (!g) {
      g = font->findGlyph(font->unknownCode());
      GX_ASSERT(g != nullptr);
    }

    if ((len + g->advX) > maxLength) { return text.substr(0, itr.pos()); }
    len += g->advX + glyphSpacing;
  }

  return text;
}
