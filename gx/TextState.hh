//
// gx/TextState.hh
// Copyright (C) 2025 Richard Bradley
//
// Internal class for text meta tag handling
//

#pragma once
#include "Color.hh"
#include "Types.hh"
#include <array>

namespace gx {
  class TextState;
}


class gx::TextState
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

 private:
  std::array<RGBA8,6> _colorStack;
  uint32_t _colors = 0;
  uint32_t _underline = 0;
};
