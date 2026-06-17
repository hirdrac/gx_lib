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
#include "IDRegion.hh"
#include "Assert.hh"
using namespace gx;


std::pair<float,float> TextFormat::calcSize(std::string_view text) const
{
  if (text.empty()) { return {0,0}; }

  GX_ASSERT(font != nullptr);
  TextMetaState ts;
  float width = 0, height = -lineSpacing;
  const float lh = float(font->size()) + lineSpacing;

  for (LineIterator lineItr{text}; lineItr; ++lineItr) {
    const auto line = *lineItr;

    float len = 0;
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
          len = (std::floor(len / tabWidth) + 1.0f) * tabWidth;
          continue;
        }
      }

      const Glyph* g = font->findGlyph(ch);
      if (!g) {
        g = font->findGlyph(font->unknownCode());
        GX_ASSERT(g != nullptr);
      }

      len += g->advX + glyphSpacing;
    }

    width = std::max(width, len - glyphSpacing);
    height += lh;
  }

  return {width, std::max(height, 0.0f)};
}

std::pair<float,float> TextFormat::calcSizeAndRegions(
  Vec2 pos, Align align, std::string_view text, IDRegionList& regions) const
{
  // TODO: adjust region height by lineSpacing

  if (text.empty()) { return {0,0}; }

  GX_ASSERT(font != nullptr);
  const float fs = float(font->size());
  const float lh = fs + lineSpacing;
  const Align h_align = hAlign(align);
  const Align v_align = vAlign(align);

  pos -= glyphY * font->ymax();
  if (v_align == Align::top) {
    pos += advY * font->ymax();
  } else if (v_align == Align::bottom) {
    pos += advY * (font->ymin() - (lh * float(lineCount(text) - 1)));
  } else if (v_align == Align::vcenter) {
    pos += advY * ((font->ymax() - (lh * float(lineCount(text) - 1))) * .5f);
  }

  TextMetaState ts;
  float width = 0, height = -lineSpacing;
  uint64_t activeID = 0;

  for (LineIterator lineItr{text}; lineItr; ++lineItr, pos += advY * lh) {
    const auto line = *lineItr;
    if (line.empty()) { continue; }

    float offset = 0;
    if (h_align != Align::left) {
      const auto [sizeW,sizeH] = calcSize(line);
      offset = (h_align == Align::right) ? sizeW : (sizeW * .5f);
    }

    float regionStart = 0, len = 0;
    for (UTF8Iterator itr{line}; itr; ++itr) {
      int ch = *itr;
      if (ch == startTag && startTag != 0) {
        const std::size_t startPos = itr.nextPos();
        const std::size_t endPos = findUTF8(line, endTag, startPos);
        if (endPos != std::string_view::npos) {
          const auto tag = line.substr(startPos, endPos - startPos);
          const auto tagType = ts.parseTag(tag);
          if (tagType != TAG_unknown) {
            if (tagType == TAG_id) {
              const uint64_t id = ts.activeID();
              if (activeID != 0) {
                // end region
                const Vec2 s = pos + (advX * (regionStart - offset));
                const Vec2 e = pos + (advX * (len - offset)) + (advY * fs);
                regions.add(activeID, s, e);
              }
              if (id != 0) {
                // start region
                regionStart = len;
              }
              activeID = id;
            }

            itr.setPos(endPos);
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
      }

      const Glyph* g = font->findGlyph(ch);
      if (!g) {
        g = font->findGlyph(font->unknownCode());
        GX_ASSERT(g != nullptr);
      }

      len += g->advX + glyphSpacing;
    }

    width = std::max(width, len - glyphSpacing);
    height += lh;

    if (activeID != 0) {
      // end region
      const Vec2 s = pos + (advX * (regionStart - offset));
      const Vec2 e = pos + (advX * (len - offset)) + (advY * fs);
      regions.add(activeID, s, e);
    }
  }

  return {width, std::max(height, 0.0f)};
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
