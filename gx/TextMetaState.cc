//
// gx/TextMetaState.cc
// Copyright (C) 2026 Richard Bradley
//

#include "TextMetaState.hh"
#include "StringUtil.hh"
#include <charconv>
using namespace gx;


template<class T>
[[nodiscard]] static T toNumber(std::string_view v)
{
  T result = 0;
  const char* d = v.data();
  std::from_chars(d, d + v.size(), result);
  return result;
}

TextMetaTagType TextMetaState::parseTag(std::string_view tag)
{
  const std::string tagLC = toLower(tag);
  if (tagLC.substr(0, 6) == "color=") {
    const auto color = parseColorStr(trimSpaces(tagLC.substr(6)));
    if (color.has_value()) {
      pushColor(color.value());
      return TAG_color;
    } else {
      return TAG_unknown;
    }
  } else if (tagLC.substr(0, 3) == "id=") {
    _id = toNumber<int64_t>(tagLC.substr(3));
    return (_id == 0) ? TAG_unknown : TAG_id;
  }

  switch (hashStr(tagLC)) {
    case hashStr("/color"):
      return popColor() ? TAG_color : TAG_unknown;
    case hashStr("ul"):
      pushUnderline();
      return TAG_underline;
    case hashStr("/ul"):
      return popUnderline() ? TAG_underline : TAG_unknown;
    case hashStr("/id"):
      _id = 0;
      return TAG_id;
    default:
      return TAG_unknown;
  }
}
