//
// gx/TextFormat.cc
// Copyright (C) 2025 Richard Bradley
//

// TODO: use advX,advY,glyphX,glyphY for calcSize(),fixText()

#include "TextFormat.hh"
#include "Font.hh"
#include "Unicode.hh"
#include "StringUtil.hh"
#include "Assert.hh"
#include <optional>
using namespace gx;


std::pair<float,float> TextFormat::calcSize(std::string_view text) const
{
  GX_ASSERT(font != nullptr);

  TextState ts;
  std::size_t lineStart = 0;
  float maxWidth = 0, height = -lineSpacing;

  while (lineStart < text.size()) {
    auto nlPos = text.find('\n', lineStart);
    if (nlPos == std::string_view::npos) { nlPos = text.size(); }

    auto line = text.substr(lineStart, nlPos - lineStart);
    //while (!line.empty() && (line.back() == ' ' || line.back() == '\t')) {
    //  line.remove_suffix(1); }

    float width = 0;
    for (UTF8Iterator itr{line}; itr; ++itr) {
      int32_t ch = *itr;
      if (ch == startTag && startTag != 0) {
        const std::size_t tagStart = itr.nextPos();
        const std::size_t tagEnd = findUTF8(line, endTag, tagStart);
        if (tagEnd != std::string_view::npos) {
          const auto tag = line.substr(tagStart, tagEnd - tagStart);
          if (parseTag(ts, tag) != TAG_unknown) {
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
    lineStart = nlPos + 1;
  }

  return {maxWidth, std::max(height, 0.0f)};
}

std::string_view TextFormat::fitText(
  std::string_view text, float maxWidth) const
{
  GX_ASSERT(font != nullptr);
  maxWidth += glyphSpacing;

  const auto nlPos = text.find('\n');
  const auto line = (nlPos == std::string_view::npos)
    ? text : text.substr(0, nlPos);

  TextState ts;
  float width = 0;

  for (UTF8Iterator itr{line}; itr; ++itr) {
    int32_t ch = *itr;
    if (ch == startTag && startTag != 0) {
      const std::size_t tagStart = itr.nextPos();
      const std::size_t tagEnd = findUTF8(line, endTag, tagStart);
      if (tagEnd != std::string_view::npos) {
        const auto tag = line.substr(tagStart, tagEnd - tagStart);
        if (parseTag(ts, tag) != TAG_unknown) {
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

[[nodiscard]] static constexpr int hexDigitVal(int val)
{
  if (val >= '0' && val <= '9') { return val - '0'; }
  if (val >= 'A' && val <= 'F') { return (val - 'A') + 10; }
  if (val >= 'a' && val <= 'f') { return (val - 'a') + 10; }
  return -1;
}

[[nodiscard]] static std::optional<RGBA8> parseColorStr(std::string_view str)
{
  if (str.empty()) { return std::nullopt; }

  if (str[0] == '#') {
    if (str.size() != 7 && str.size() != 9) { return std::nullopt; }

    int vals[8]{0,0,0,0,0,0,15,15};
    const std::size_t num = str.size() - 1;
    for (std::size_t i = 0; i < num; ++i) {
      vals[i] = hexDigitVal(str[i+1]);
      if (vals[i] < 0) { return std::nullopt; }
    }

    return packRGBA8i(uint8_t((vals[0]<<4) + vals[1]),
                      uint8_t((vals[2]<<4) + vals[3]),
                      uint8_t((vals[4]<<5) + vals[5]),
                      uint8_t((vals[6]<<4) + vals[7]));
  }

  switch (hashStrLC(str)) {
    case hashStrLC("white"):   return packRGBA8(WHITE);
    case hashStrLC("black"):   return packRGBA8(BLACK);
    case hashStrLC("gray25"):  return packRGBA8(GRAY25);
    case hashStrLC("gray50"):  return packRGBA8(GRAY50);
    case hashStrLC("gray75"):  return packRGBA8(GRAY75);
    case hashStrLC("red"):     return packRGBA8(RED);
    case hashStrLC("green"):   return packRGBA8(GREEN);
    case hashStrLC("blue"):    return packRGBA8(BLUE);
    case hashStrLC("cyan"):    return packRGBA8(CYAN);
    case hashStrLC("yellow"):  return packRGBA8(YELLOW);
    case hashStrLC("magenta"): return packRGBA8(MAGENTA);
    default:                   return std::nullopt;
  }
}

gx::TextMetaTagType TextFormat::parseTag(
  TextState& ts, std::string_view tag) const
{
  if (tag.substr(0, 6) == "color=") {
    const auto color = parseColorStr(trimSpaces(tag.substr(6)));
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
