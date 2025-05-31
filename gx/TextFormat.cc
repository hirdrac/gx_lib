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
using namespace gx;


float TextFormat::calcLength(std::string_view text) const
{
  GX_ASSERT(font != nullptr);

  TextState ts;
  float max_len = 0, len = 0;
  for (UTF8Iterator itr{text}; itr; ++itr) {
    int32_t ch = *itr;
    if (ch == startTag) {
      const std::size_t startPos = itr.pos() + 1;
      auto line = text.substr(startPos, text.find('\n', startPos+1));
      const std::size_t endPos = findUTF8(line, endTag);
      if (endPos != std::string_view::npos) {
        auto tag = line.substr(0, endPos);
        if (parseTag(ts, tag)) {
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
    if (ch == startTag) {
      const std::size_t startPos = itr.pos() + 1;
      auto line = text.substr(startPos, text.find('\n', startPos+1));
      const std::size_t endPos = findUTF8(line, endTag);
      if (endPos != std::string_view::npos) {
        auto tag = line.substr(0, endPos);
        if (parseTag(ts, tag)) {
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

bool TextFormat::parseTag(TextState& ts, std::string_view tag) const
{
  switch (hashStr(tag)) {
    case hashStr("white"):   ts.color = packRGBA8(WHITE); break;
    case hashStr("black"):   ts.color = packRGBA8(BLACK); break;
    case hashStr("gray25"):  ts.color = packRGBA8(GRAY25); break;
    case hashStr("gray50"):  ts.color = packRGBA8(GRAY50); break;
    case hashStr("gray75"):  ts.color = packRGBA8(GRAY75); break;
    case hashStr("red"):     ts.color = packRGBA8(RED); break;
    case hashStr("green"):   ts.color = packRGBA8(GREEN); break;
    case hashStr("blue"):    ts.color = packRGBA8(BLUE); break;
    case hashStr("cyan"):    ts.color = packRGBA8(CYAN); break;
    case hashStr("yellow"):  ts.color = packRGBA8(YELLOW); break;
    case hashStr("magenta"): ts.color = packRGBA8(MAGENTA); break;
    default: return false;
  }

  return true;
}
