//
// gx/TextFormat.hh
// Copyright (C) 2024 Richard Bradley
//

#pragma once
#include "Types.hh"


namespace gx {
  class Font;
  struct TextFormat;
}

struct gx::TextFormat
{
  const Font* font = nullptr;
  float lineSpacing = 0;  // extra spacing between lines
  float glyphSpacing = 0; // extra spacing between glyphs
  float tabWidth = 0;     // pixel width of tabs
  Vec2 advX{1,0};         // dir of next glyph
  Vec2 advY{0,1};         // dir of next line
  Vec2 glyphX{1,0};       // glyph quad sides
  Vec2 glyphY{0,1};

  // transforms
  void scaleX(float s) { advX *= s; glyphX *= s; }
  void scaleY(float s) { advY *= s; glyphY *= s; }
  void scale(float s) { scaleX(s); scaleY(s); }

  void rotate(float rad) {
    advX = gx::rotate(advX, rad);
    advY = gx::rotate(advY, rad);
    glyphX = gx::rotate(glyphX, rad);
    glyphY = gx::rotate(glyphY, rad);
  }

  [[nodiscard]] float calcLength(std::string_view text) const;
};
