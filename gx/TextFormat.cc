//
// gx/TextFormat.cc
// Copyright (C) 2026 Richard Bradley
//

// TODO: use advX,advY,glyphX,glyphY for calcSize(),fixText()

#include "TextFormat.hh"
#include "TextMetaState.hh"
#include "StringUtil.hh"
#include "Font.hh"
#include "Unicode.hh"
#include "Assert.hh"
using namespace gx;


std::pair<float,float> TextFormat::calcSize(std::string_view text) const
{
  GX_ASSERT(font != nullptr);

  TextMetaState ts;
  float maxWidth = 0, height = -lineSpacing;

  for (LineIterator lineItr{text}; lineItr; ++lineItr) {
    const auto line = *lineItr;

    float width = 0;
    for (UTF8Iterator itr{line}; itr; ++itr) {
      int32_t ch = *itr;
      if (ch == startTag && startTag != 0) {
        const std::size_t tagStart = itr.nextPos();
        const std::size_t tagEnd = findUTF8(line, endTag, tagStart);
        if (tagEnd != std::string_view::npos) {
          const auto tag = line.substr(tagStart, tagEnd - tagStart);
          if (ts.parseTag(tag) != TAG_unknown) {
            itr.setPos(tagEnd);
            continue;
          }
        }
      } else if (ch == '\t') {
        if (tabWidth <= 0) {
          ch = ' ';
        } else {
          width = (std::floor(width / tabWidth) + 1.0f) * tabWidth;
          continue;
        }
      }

      const Glyph* g = font->findGlyph(ch);
      if (!g) {
        g = font->findGlyph(font->unknownCode());
        GX_ASSERT(g != nullptr);
      }

      width += g->advX + glyphSpacing;
    }

    maxWidth = std::max(maxWidth, width - glyphSpacing);
    height += float(font->size()) + lineSpacing;
  }

  return {maxWidth, std::max(height, 0.0f)};
}

std::string_view TextFormat::fitText(
  std::string_view text, float maxWidth) const
{
  GX_ASSERT(font != nullptr);
  maxWidth += glyphSpacing;

  // first line only
  const auto line = text.substr(0, text.find('\n'));

  TextMetaState ts;
  float width = 0;

  for (UTF8Iterator itr{line}; itr; ++itr) {
    int32_t ch = *itr;
    if (ch == startTag && startTag != 0) {
      const std::size_t tagStart = itr.nextPos();
      const std::size_t tagEnd = findUTF8(line, endTag, tagStart);
      if (tagEnd != std::string_view::npos) {
        const auto tag = line.substr(tagStart, tagEnd - tagStart);
        if (ts.parseTag(tag) != TAG_unknown) {
          itr.setPos(tagEnd);
          continue;
        }
      }
    } else if (ch == '\t') {
      if (tabWidth <= 0) {
        ch = ' ';
      } else {
        width = (std::floor(width / tabWidth) + 1.0f) * tabWidth;
        continue;
      }
    }

    const Glyph* g = font->findGlyph(ch);
    if (!g) {
      g = font->findGlyph(font->unknownCode());
      GX_ASSERT(g != nullptr);
    }

    const float len = g->advX + glyphSpacing;
    if ((width + len) > maxWidth) { return text.substr(0, itr.pos()); }
    width += len;
  }

  return text;
}
