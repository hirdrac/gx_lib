//
// gx/Font.hh
// Copyright (C) 2021 Richard Bradley
//

#pragma once
#include "Glyph.hh"
#include "Texture.hh"
#include "Types.hh"
#include <string>
#include <string_view>
#include <map>


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

  bool load(const char* fileName, int fontSize);
  bool load(const std::string& fn, int fontSize) {
    return load(fn.c_str(), fontSize); }
  bool loadFromMemory(const void* mem, std::size_t memSize, int fontSize);
    // load TTF file

  bool initFromData(const GlyphStaticData* data, int glyphs, int fontSize);
    // init from static glyph data

  bool makeAtlas(Window& win);

  [[nodiscard]] int size() const { return _size; }
    // font pixel size

  [[nodiscard]] float ymin() const { return _ymin; }
  [[nodiscard]] float ymax() const { return _ymax; }
    // min/max y values relative to origin for baseline calculation
    // (only ascii alpha-numeric values used for calc)

  [[nodiscard]] const Texture& tex() const { return _tex; }
    // texture atlas created by engine

  [[nodiscard]] const Glyph* findGlyph(int code) const {
    auto i = _glyphs.find(code);
    return (i != _glyphs.end()) ? &(i->second) : nullptr;
  }

  [[nodiscard]] const auto& glyphs() const { return _glyphs; }

  [[nodiscard]] float calcWidth(std::string_view text) const;
    // single text line width calculation

  [[nodiscard]] std::size_t fitChars(
    std::string_view text, float maxWidth) const;
    // returns size of substr to fit within specified width

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
