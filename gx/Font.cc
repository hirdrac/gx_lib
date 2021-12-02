//
// gx/Font.cc
// Copyright (C) 2021 Richard Bradley
//

// TODO: investigate using stb_truetype.h
// TODO: use signed distance fields for font texture
//       (FT_RENDER_MODE_SDF available in freetype 2.11)

#include "Font.hh"
#include "Image.hh"
#include "Renderer.hh"
#include "Logger.hh"
#include <cstdlib>
#include <cstring>
#include <cassert>

#include <ft2build.h>
#include <freetype/freetype.h>
using namespace gx;


namespace {
  FT_Library ftLib = nullptr;

  void cleanupFreeType()
  {
    if (ftLib && FT_Done_FreeType(ftLib)) {
      GX_LOG_ERROR("FT_Done_FreeType() failed");
    }

    ftLib = nullptr;
  }

  bool initFreeType()
  {
    if (ftLib) { return true; }

    if (FT_Init_FreeType(&ftLib)) {
      GX_LOG_ERROR("FT_Init_FreeType() failed");
      return false;
    }

    std::atexit(cleanupFreeType);
    return true;
  }

  bool genGlyphs(Font& font, FT_Face face,
                 uint32_t start = 0, uint32_t end = 0)
  {
    // read font glyph data
    bool hasVert = FT_HAS_VERTICAL(face);
    FT_UInt index = 0;
    FT_ULong ch = FT_Get_First_Char(face, &index);

    for (; index != 0; ch = FT_Get_Next_Char(face, ch, &index)) {
      if (ch < 32 || ch < start) { continue; }
      else if (end != 0 && ch > end) { break; }

      if (FT_Load_Glyph(face, index, FT_LOAD_DEFAULT)) {
        GX_LOG_ERROR("FT_Load_Glyph() failed");
        return false;
      }

      FT_GlyphSlot gs = face->glyph;
      if (FT_Render_Glyph(gs, FT_RENDER_MODE_NORMAL)) {
        GX_LOG_ERROR("FT_Render_Glyph() failed");
        return false;
      }

      float advX = float(gs->advance.x) / 64.0f;
      float advY = hasVert ? (float(gs->advance.y) / 64.0f) : 0;
      font.addGlyph(int(ch), int(gs->bitmap.width), int(gs->bitmap.rows),
                    float(gs->bitmap_left), float(gs->bitmap_top),
                    advX, advY, gs->bitmap.buffer, true);
    }
    return true;
  }
}


bool Font::load(const char* fileName, int fontSize)
{
  if (!initFreeType()) { return false; }

  FT_Face face;
  if (FT_New_Face(ftLib, fileName, 0, &face)) {
    GX_LOG_ERROR("FT_New_Face(\"", fileName, "\") failed");
    return false;
  }

  if (FT_Set_Pixel_Sizes(face, 0, FT_UInt(fontSize))) {
    GX_LOG_ERROR("FT_Set_Pixel_Sizes(", fontSize, ") failed");
    FT_Done_Face(face);
    return false;
  }

  // read font glyph data
  init(fontSize);
  bool status = genGlyphs(*this, face);
  FT_Done_Face(face);
  return status;
}

bool Font::loadFromMemory(
  const void* mem, std::size_t memSize, int fontSize)
{
  if (!initFreeType()) { return false; }

  FT_Face face;
  if (FT_New_Memory_Face(
        ftLib, (const FT_Byte*)mem, FT_Long(memSize), 0, &face)) {
    GX_LOG_ERROR("FT_New_Memory_Face(), failed");
    return false;
  }

  if (FT_Set_Pixel_Sizes(face, 0, FT_UInt(fontSize))) {
    GX_LOG_ERROR("FT_Set_Pixel_Sizes(", fontSize, ") failed");
    FT_Done_Face(face);
    return false;
  }

  // read font glyph data
  init(fontSize);
  bool status = genGlyphs(*this, face);
  FT_Done_Face(face);
  return status;
}

bool Font::loadFromData(
  const GlyphStaticData* data, int glyphs, int fontSize)
{
  init(fontSize);
  for (int i = 0; i < glyphs; ++i) {
    const auto& d = data[i];
    addGlyph(d.code, d.width, d.height, d.left, d.top, d.advX, d.advY,
             d.bitmap, false);
  }

  return true;
}

bool Font::makeAtlas(Renderer& ren)
{
  if (_tex && !_changed) {
    return true; // atlas already set
  }

  // get font dimensions for texture creation
  int maxW = 0, maxH = 0, totalW = 0;
  for (auto& itr : _glyphs) {
    Glyph& g = itr.second;
    if (g.width > maxW) { maxW = g.width; }
    if (g.height > maxH) { maxH = g.height; }
    totalW += g.width;
  }

  // calculate texture size
  int maxSize = ren.maxTextureSize();
  int texW, texH;
  int rows = 0;
  do {
    ++rows;
    texW = maxW + ((totalW + int(_glyphs.size()) + 1) / rows);
    texH = ((maxH+1) * rows) + 1;
  } while ((texW > texH*2) || (texW > maxSize));

  // round up texture size
  if (texW & 15) { texW = (texW & ~15) + 16; }
  if (texH & 15) { texH = (texH & ~15) + 16; }

  Image atlas;
  atlas.init(texW, texH, 1);

  int x = 1, y = 1;
  for (auto& itr : _glyphs) {
    Glyph& g = itr.second;
    if (g.width == 0 || g.height == 0) {
      g.t0 = {};
      g.t1 = {};
      continue;
    }

    if ((x+g.width) >= texW) { x = 1; y += maxH + 1; }

    // texture coords not normalized with TEXTURE_RECTANGLE
    // (y coord flipped to match GL coords)
    g.t0.x = float(x) / float(texW);
    g.t0.y = float(y) / float(texH);
    g.t1.x = float(x + g.width) / float(texW);
    g.t1.y = float(y + g.height) / float(texH);

    atlas.stamp(x, y, g);
    x += g.width + 1;
  }

  if (_tex) {
    _tex.update(atlas);
  } else {
    _tex.init(ren, atlas);
  }

  _changed = false;
  return true;
}

void Font::addGlyph(
  int code, int width, int height, float left, float top,
  float advX, float advY, const uint8_t* bitmap, bool copy)
{
  _changed = true;
  Glyph& g = newGlyph(code, width, height, left, top, advX, advY);
  std::size_t size = g.width * g.height;
  if (copy && bitmap && size > 0) {
    g.bitmap_copy.reset(new uint8_t[size]);
    std::memcpy(g.bitmap_copy.get(), bitmap, size);
    g.bitmap = g.bitmap_copy.get();
  } else {
    g.bitmap_copy.reset(nullptr);
    g.bitmap = bitmap;
  }
}

void Font::init(int fontSize)
{
  _size = fontSize;
  _ymin = 0;
  _ymax = 0;
  _digitWidth = 0;
  _glyphs.clear();
}

Glyph& Font::newGlyph(
  int code, int width, int height, float left, float top, float advX, float advY)
{
  assert(width >= 0 && width < 0xffff);
  assert(height >= 0 && height < 0xffff);

  Glyph& g = _glyphs[code];
  g.width = uint16_t(width);
  g.height = uint16_t(height);
  g.left = left;
  g.top = top;
  g.advX = advX;
  g.advY = advY;

  if ((code > 47 && code < 94) || (code > 96 && code < 127)) {
    // ymin/ymax adjust for a limited range of characters
    float yt = top;
    if (yt > _ymax) { _ymax = yt; }
    float yb = top - float(height);
    if (yb < _ymin) { _ymin = yb; }
  }

  if (std::isdigit(code) || code == '.' || code == '-') {
    _digitWidth = std::max(
      _digitWidth, std::max(float(g.width + g.left), g.advX));
  }

  return g;
}

float Font::calcWidth(int code) const
{
  const Glyph* g = findGlyph(code);
  return g ? std::max(g->advX, g->width + g->left) : 0;
}

float Font::calcWidth(std::string_view text) const
{
  float max_width = 0, width = 0;
  const Glyph* g = nullptr;
  for (auto ch : text) {
    if (ch == '\t') {
      ch = ' '; // tab logic should be handled outside of thie function
    } else if (ch == '\n') {
      //if (g) { width += -g->advX + (float(g->width) + g->left); }
      max_width = std::max(max_width, width);
      width = 0;
    }
    g = findGlyph(ch);
    if (g) { width += g->advX; }
  }

  //if (g) { width += -g->advX + (float(g->width) + g->left); }
  return std::max(max_width, width);
}

std::size_t Font::fitChars(std::string_view text, float maxWidth) const
{
  float w = 0.0f;
  for (size_t i = 0; i < text.size(); ++i) {
    int ch = text[i];
    if (ch == '\t') { ch = ' '; }
    const Glyph* g = findGlyph(ch);
    if (!g) { continue; }

    if ((w + g->advX) > maxWidth) { return i; }
    w += g->advX;
  }

  return text.size();
}
