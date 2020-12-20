//
// gx/Font.hh
// Copyright (C) 2020 Richard Bradley
//

#pragma once
#include "Glyph.hh"
#include "Texture.hh"
#include "Types.hh"
#include <string>
#include <string_view>
#include <map>
#include <cstdint>


namespace gx {
  struct GlyphStaticData {
    int32_t code;
    uint16_t width, height;
    float left, top, advX, advY;
    const uint8_t* bitmap;
  };

  class Image;
  class Font;
  class Window;
}

class gx::Font
{
 public:
  Font() = default;
  Font(Font&&) noexcept = default;
  Font& operator=(Font&&) noexcept = default;

  bool init(const char* fileName, int fontSize);
  bool init(const std::string& fn, int fontSize) {
    return init(fn.c_str(), fontSize); }

  bool init(const GlyphStaticData* data, int glyphs, int fontSize);

  bool makeAtlas(Window& win);

  int size() const { return _size; }
    // font pixel size

  float ymin() const { return _ymin; }
  float ymax() const { return _ymax; }
    // min/max y values relative to origin for baseline calculation
    // (only ascii alpha-numeric values used for calc)

  const Texture& tex() const { return _tex; }
    // texture atlas created by engine

  const Glyph* findGlyph(int code) const {
    auto i = _glyphs.find(code);
    return (i != _glyphs.end()) ? &(i->second) : nullptr;
  }

  float calcWidth(std::string_view text) const;
    // single text line width calculation

  int calcLines(std::string_view text) const;

  void setSize(int s) { _size = s; }
  void setYmin(float y) { _ymin = y; }
  void setYmax(float y) { _ymax = y; }

  void addGlyph(int code, int width, int height, float left, float top,
		float advX, float advY, const uint8_t* bitmap, bool copy);
  void addGlyph(int code, int width, int height, float left, float top,
                float advX, float advY, std::unique_ptr<uint8_t[]>&& bitmap);

 private:
  std::map<int,Glyph> _glyphs;
  Texture _tex;
  int _size = 0;
  float _ymin = 0, _ymax = 0;
  bool _changed = false; // change since last atlas creation

  Glyph& newGlyph(int code, int width, int height, float left, float top,
		  float advX, float advY);

  // prevent copy/assignment
  Font(const Font&) = delete;
  Font& operator=(const Font&) = delete;
};
