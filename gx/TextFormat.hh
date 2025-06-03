//
// gx/TextFormat.hh
// Copyright (C) 2025 Richard Bradley
//

#pragma once
#include "Color.hh"
#include "Types.hh"
#include <string_view>
#include <array>

namespace gx {
  class TextState;
  struct TextFormat;

  enum TextMetaTagType {
    TAG_unknown = 0,
    TAG_color = 1
  };
}


class gx::TextState
{
 public:
  [[nodiscard]] RGBA8 color() const {
    return (_colors > 0) ? _colorStack[_colors - 1] : 0;
  }

  void pushColor(RGBA8 c) {
    _colorStack[_colors] = c;
    if (_colors < (_colorStack.size() - 1)) { ++_colors; }
  }

  bool popColor() {
    if (_colors == 0) { return false; }
    --_colors; return true;
  }

 private:
  std::array<RGBA8,7> _colorStack;
  uint32_t _colors = 0;
};


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

  int32_t startTag = '<'; // meta-tag start char
  int32_t endTag = '>';   // meta-tag end char

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
    // returns pixel length of longest line in input text

  [[nodiscard]] std::string_view fitText(
    std::string_view text, float maxLength) const;
    // returns sub-string that fits in specified length

  TextMetaTagType parseTag(TextState& ts, std::string_view tag) const;
    // returns true if text is a valid text meta tag & updates TextState
};
