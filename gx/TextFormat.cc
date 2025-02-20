//
// gx/TextFormat.cc
// Copyright (C) 2025 Richard Bradley
//

// TODO: add width/height calc for multi-line text

#include "TextFormat.hh"
#include "Font.hh"
#include "Unicode.hh"
#include "Assert.hh"
using namespace gx;


float TextFormat::calcLength(std::string_view text) const
{
  GX_ASSERT(font != nullptr);

  float max_len = 0, len = 0;
  bool inTag = false;
  for (UTF8Iterator itr{text}; itr; ++itr) {
    int32_t ch = *itr;
    if (inTag) {
      if (ch == endTag) { inTag = false; }
      continue;
    } else if (ch == startTag) {
      if (!++itr) {
        break;
      } else if (*itr != startTag) {
        inTag = true;
        continue;
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

int TextFormat::countLines(std::string_view text) const
{
  if (text.empty()) { return 0; }

  int lines = 1;
  bool inTag = false;
  for (UTF8Iterator itr{text}; itr; ++itr) {
    const int32_t ch = *itr;
    if (inTag) {
      if (ch == endTag) { inTag = false; }
      continue;
    } else if (ch == startTag) {
      if (!++itr) {
        break;
      } else if (*itr != startTag) {
        inTag = true;
        continue;
      }
    } else if (ch == '\n') {
      ++lines;
    }
  }
  return lines;
}

std::size_t TextFormat::find(
  std::string_view text, int32_t val, std::size_t pos) const
{
  bool inTag = false;
  for (UTF8Iterator itr{text}; itr; ++itr) {
    if (itr.pos() < pos) { continue; }

    const int ch = *itr;
    if (inTag) {
      if (ch == endTag) { inTag = false; }
      continue;
    } else if (ch == startTag) {
      if (!++itr) {
        break;
      } else if (*itr != startTag) {
        inTag = true;
        continue;
      }
    } else if (ch == '\n') {
      return itr.pos();
    }
  }
  return std::string_view::npos;
}

std::string_view TextFormat::fitText(
  std::string_view text, float maxLength) const
{
  GX_ASSERT(font != nullptr);
  maxLength += glyphSpacing;

  float len = 0;
  bool inTag = false;
  for (UTF8Iterator itr{text}; itr; ++itr) {
    int32_t ch = *itr;
    if (inTag) {
      if (ch == endTag) { inTag = false; }
      continue;
    } else if (ch == startTag) {
      if (!++itr) {
        break;
      } else if (*itr != startTag) {
        inTag = true;
        continue;
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
