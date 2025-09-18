//
// gx/TextFormat.cc
// Copyright (C) 2025 Richard Bradley
//

// TODO: add width/height calc for multi-line text

#include "TextFormat.hh"
#include "Font.hh"
#include "Unicode.hh"
#include "StringUtil.hh"
#include "Assert.hh"
#include <optional>
using namespace gx;


float TextFormat::calcLength(std::string_view text) const
{
  GX_ASSERT(font != nullptr);

  TextState ts;
  float max_len = 0, len = 0;
  for (UTF8Iterator itr{text}; itr; ++itr) {
    int32_t ch = *itr;
    if (ch == startTag && startTag != 0) {
      const std::size_t startPos = itr.pos() + 1;
      const auto line = text.substr(startPos, text.find('\n', startPos+1));
      const std::size_t endPos = findUTF8(line, endTag);
      if (endPos != std::string_view::npos) {
        const auto tag = line.substr(0, endPos);
        if (parseTag(ts, tag) != TAG_unknown) {
          itr.setPos(startPos + endPos);
          continue;
        }
      }
    } else if (ch == '\t') {
      if (tabWidth <= 0) {
        ch = ' ';
      } else {
        len = (std::floor(len / tabWidth) + 1.0f) * tabWidth;
        continue;
      }
    } else if (ch == '\n') {
      max_len = std::max(max_len, len);
      len = 0;
    }

    const Glyph* g = font->findGlyph(ch);
    if (!g) {
      g = font->findGlyph(font->unknownCode());
      GX_ASSERT(g != nullptr);
    }

    len += g->advX + glyphSpacing;
  }

  return std::max(max_len, len - glyphSpacing);
}

std::string_view TextFormat::fitText(
  std::string_view text, float maxLength) const
{
  GX_ASSERT(font != nullptr);
  maxLength += glyphSpacing;

  TextState ts;
  float len = 0;
  for (UTF8Iterator itr{text}; itr; ++itr) {
    int32_t ch = *itr;
    if (ch == startTag && startTag != 0) {
      const std::size_t startPos = itr.pos() + 1;
      const auto line = text.substr(startPos, text.find('\n', startPos+1));
      const std::size_t endPos = findUTF8(line, endTag);
      if (endPos != std::string_view::npos) {
        const auto tag = line.substr(0, endPos);
        if (parseTag(ts, tag) != TAG_unknown) {
          itr.setPos(startPos + endPos);
          continue;
        }
      }
    } else if (ch == '\t') {
      if (tabWidth <= 0) {
        ch = ' ';
      } else {
        len = (std::floor(len / tabWidth) + 1.0f) * tabWidth;
        continue;
      }
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

[[nodiscard]] static std::optional<RGBA8> parseColorStr(std::string_view str)
{
  // TODO: add parsing hex color value

  switch (hashStr(str)) {
    case hashStr("white"):   return packRGBA8(WHITE);
    case hashStr("black"):   return packRGBA8(BLACK);
    case hashStr("gray25"):  return packRGBA8(GRAY25);
    case hashStr("gray50"):  return packRGBA8(GRAY50);
    case hashStr("gray75"):  return packRGBA8(GRAY75);
    case hashStr("red"):     return packRGBA8(RED);
    case hashStr("green"):   return packRGBA8(GREEN);
    case hashStr("blue"):    return packRGBA8(BLUE);
    case hashStr("cyan"):    return packRGBA8(CYAN);
    case hashStr("yellow"):  return packRGBA8(YELLOW);
    case hashStr("magenta"): return packRGBA8(MAGENTA);
    default:                 return std::nullopt;
  }
}

gx::TextMetaTagType TextFormat::parseTag(
  TextState& ts, std::string_view tag) const
{
  if (tag.substr(0, 6) == "color=") {
    const auto color = parseColorStr(tag.substr(6));
    if (color.has_value()) {
      ts.pushColor(color.value());
      return TAG_color;
    } else {
      return TAG_unknown;
    }
  }

  switch (hashStr(tag)) {
    case hashStr("/color"):
      return ts.popColor() ? TAG_color : TAG_unknown;
    case hashStr("ul"):
      ts.pushUnderline();
      return TAG_underline;
    case hashStr("/ul"):
      return ts.popUnderline() ? TAG_underline : TAG_unknown;
    default:
      return TAG_unknown;
  }
}
