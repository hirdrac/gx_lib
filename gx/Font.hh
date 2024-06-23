//
// gx/Font.hh
// Copyright (C) 2024 Richard Bradley
//
// Create texture atlas for font glyph rendering
//

#pragma once
#include "Renderer.hh"
#include "Glyph.hh"
#include "Types.hh"
#include <string_view>
#include <map>


namespace gx {
  struct GlyphStaticData {
    int32_t code;
    uint16_t width, height;
    float left, top, advX, advY;
    const uint8_t* bitmap;
  };

  class Font;
  class Renderer;
}

class gx::Font
{
 public:
  Font() = default;
  explicit Font(int fontSize) : _size{fontSize} { }

  // prevent copy but allow move
  Font(const Font&) = delete;
  Font& operator=(const Font&) = delete;
  Font(Font&&) noexcept = default;
  Font& operator=(Font&&) noexcept = default;

  void setSize(int s) { _size = s; }
  [[nodiscard]] int size() const { return _size; }
    // font pixel size

  bool load(const char* fileName);

  template<class T>
  bool load(const T& fn) { return load(fn.c_str()); }

  bool loadFromMemory(const void* mem, std::size_t memSize);
    // load TTF file & render glyphs for current size

  bool loadFromData(const GlyphStaticData* data, int glyphs);
    // load from static glyph data

  bool makeAtlas(Renderer& ren);

  template<class T>
  bool makeAtlas(T& win) { return makeAtlas(win.renderer()); }

  [[nodiscard]] float ymin() const { return _ymin; }
  [[nodiscard]] float ymax() const { return _ymax; }
    // min/max y values relative to origin for baseline calculation
    // (only ascii alpha-numeric values used for calc)

  [[nodiscard]] float digitWidth() const { return _digitWidth; }
    // max width of 0123456789-. characters

  [[nodiscard]] const TextureHandle& atlas() const { return _atlas; }
  [[nodiscard]] int atlasWidth() const { return _atlasWidth; }
  [[nodiscard]] int atlasHeight() const { return _atlasHeight; }
    // texture atlas created by engine

  [[nodiscard]] const auto& glyphs() const { return _glyphs; }

  [[nodiscard]] bool empty() const { return _glyphs.empty(); }
  [[nodiscard]] explicit operator bool() const { return !empty(); }

  [[nodiscard]] const Glyph* findGlyph(int code) const {
    auto i = _glyphs.find(code);
    return (i != _glyphs.end()) ? &(i->second) : nullptr;
  }

  [[nodiscard]] float glyphWidth(int code) const {
    const Glyph* g = findGlyph(code);
    return g ? std::max(g->advX, g->width + g->left) : 0;
  }

  [[nodiscard]] float calcLength(
    std::string_view text, float glyphSpacing) const;
    // returns pixel length of longest line in input text

  [[nodiscard]] std::string_view fitText(
    std::string_view text, float maxWidth) const;
    // returns sub-string that fits in specified width

  void addGlyph(int code, int width, int height, float left, float top,
		float advX, float advY, const uint8_t* bitmap, bool copy);

  [[nodiscard]] int32_t unknownCode() const { return _unknownCode; }
  void setUnknownCode(int32_t uc) { _unknownCode = uc; }
    // read/set alternate glyph code to use for unknown code values

 private:
  std::map<int,Glyph> _glyphs;
  TextureHandle _atlas;
  int _atlasWidth;
  int _atlasHeight;
  int _size = 0;
  float _ymin = 0, _ymax = 0;
  float _digitWidth = 0;
  int32_t _unknownCode = '*';
  bool _changed = false; // change since last atlas creation

  Glyph& newGlyph(int code, int width, int height, float left, float top,
		  float advX, float advY);
  void calcAttributes();
};
