//
// gx/TextMetaState.hh
// Copyright (C) 2026 Richard Bradley
//
// Text meta tag parsing & tag state handling
//
// supported tags:
//   <color=X></color>  named color or hex color value
//   <ul></ul>          underline text
//   <id=X></id>        specify selectable text w/ ID number
//

#pragma once
#include "Color.hh"
#include "Types.hh"
#include <array>

namespace gx {
  enum TextMetaTagType {
    TAG_unknown = 0,
    TAG_color = 1,
    TAG_underline = 2,
    TAG_id = 3,
  };
}


class gx::TextMetaState
{
 public:
  // color
  [[nodiscard]] uint32_t colorCount() const { return _colors; }
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

  // underline
  [[nodiscard]] bool underline() const { return _underline > 0; }

  void pushUnderline() { ++_underline; }

  bool popUnderline() {
    if (_underline == 0) { return false; }
    --_underline; return true;
  }

  TextMetaTagType parseTag(std::string_view tag);
    // returns type of text meta tag & updates state

  [[nodiscard]] int64_t activeID() const { return _id; }
    // current value of id tag

 private:
  std::array<RGBA8,8> _colorStack;
  uint32_t _colors = 0;
  uint32_t _underline = 0;
  int64_t _id = 0;
};
