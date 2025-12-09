//
// gx/TextMetaState.cc
// Copyright (C) 2025 Richard Bradley
//

#include "TextMetaState.hh"
#include "StringUtil.hh"
#include <optional>
using namespace gx;


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

TextMetaTagType TextMetaState::parseTag(std::string_view tag)
{
  std::string tagLC = toLower(tag);
  if (tagLC.substr(0, 6) == "color=") {
    const auto color = parseColorStr(trimSpaces(tagLC.substr(6)));
    if (color.has_value()) {
      pushColor(color.value());
      return TAG_color;
    } else {
      return TAG_unknown;
    }
  }

  switch (hashStr(tagLC)) {
    case hashStr("/color"):
      return popColor() ? TAG_color : TAG_unknown;
    case hashStr("ul"):
      pushUnderline();
      return TAG_underline;
    case hashStr("/ul"):
      return popUnderline() ? TAG_underline : TAG_unknown;
    default:
      return TAG_unknown;
  }
}
