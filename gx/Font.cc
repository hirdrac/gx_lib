//
// gx/Font.cc
// Copyright (C) 2020 Richard Bradley
//
// TODO - investigate using stb_truetype.h

#include "Font.hh"
#include "Image.hh"
#include "Window.hh"
#include "Logger.hh"
#include <string>
#include <cstdlib>

#include <ft2build.h>
#include <freetype/freetype.h>


namespace {
  FT_Library ftLib = nullptr;

  void cleanupFreeType()
  {
    if (ftLib && FT_Done_FreeType(ftLib)) {
      LOG_ERROR("FT_Done_FreeType() failed");
    }

    ftLib = nullptr;
  }

  int initFreeType()
  {
    if (FT_Init_FreeType(&ftLib)) {
      LOG_ERROR("FT_Init_FreeType() failed");
      return -1;
    }

    std::atexit(cleanupFreeType);
    return 0;
  }
}


bool gx::Font::init(const char* fileName, int fontSize)
{
  if (!ftLib) {
    int error = initFreeType();
    if (error) { return false; }
  }

  // range of characters (make configurable?)
  uint32_t start = 0;
  uint32_t end = 0; // no end

  FT_Face face;
  if (FT_New_Face(ftLib, fileName, 0, &face)) {
    LOG_ERROR("FT_New_Face(\"", fileName, "\") failed");
    return false;
  }

  if (FT_Set_Pixel_Sizes(face, 0, fontSize)) {
    LOG_ERROR("FT_Set_Pixel_Sizes(", fontSize, ") failed");
    return false;
  }

  // read font glyph data
  bool hasVert = FT_HAS_VERTICAL(face);
  FT_UInt index = 0;
  FT_ULong ch = FT_Get_First_Char(face, &index);

  for (; index != 0; ch = FT_Get_Next_Char(face, ch, &index)) {
    if (ch < 32 || ch < start) { continue; }
    else if (end != 0 && ch > end) { break; }

    if (FT_Load_Glyph(face, index, FT_LOAD_DEFAULT)) {
      LOG_ERROR("FT_Load_Glyph() failed");
      return -1;
    }

    FT_GlyphSlot gs = face->glyph;
    if (FT_Render_Glyph(gs, FT_RENDER_MODE_NORMAL)) {
      LOG_ERROR("FT_Render_Glyph() failed");
      return -1;
    }

    float advX = float(gs->advance.x) / 64.0f;
    float advY = hasVert ? (float(gs->advance.y) / 64.0f) : 0;
    addGlyph(ch, gs->bitmap.width, gs->bitmap.rows, gs->bitmap_left,
	     gs->bitmap_top, advX, advY, gs->bitmap.buffer, true);
  }

  _size = fontSize;
  return true;
}

bool gx::Font::init(const GlyphStaticData* data, int glyphs, int fontSize)
{
  for (int i = 0; i < glyphs; ++i) {
    const auto& d = data[i];
    addGlyph(d.code, d.width, d.height, d.left, d.top, d.advX, d.advY,
             d.bitmap, false);
  }

  _size = fontSize;
  return true;
}

bool gx::Font::makeAtlas(Window& win)
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
  int maxSize = win.maxTextureSize();
  int texW, texH;
  int rows = 0;
  do {
    ++rows;
    texW = maxW + ((totalW + _glyphs.size() + 1) / rows);
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
    _tex.init(win, atlas);
  }

  _changed = false;
  return true;
}

void gx::Font::addGlyph(
  int code, int width, int height, float left, float top,
  float advX, float advY, const uint8_t* bitmap, bool copy)
{
  _changed = true;
  Glyph& g = newGlyph(code, width, height, left, top, advX, advY);
  int size = g.width * g.height;
  if (copy && bitmap && size > 0) {
    g.bitmap_copy.reset(new uint8_t[size]);
    std::memcpy(g.bitmap_copy.get(), bitmap, size);
    g.bitmap = g.bitmap_copy.get();
  } else {
    g.bitmap_copy.reset(nullptr);
    g.bitmap = bitmap;
  }
}

void gx::Font::addGlyph(
  int code, int width, int height, float left, float top,
  float advX, float advY, std::unique_ptr<uint8_t[]>&& bitmap)
{
  _changed = true;
  Glyph& g = newGlyph(code, width, height, left, top, advX, advY);
  g.bitmap_copy = std::move(bitmap);
  g.bitmap = g.bitmap_copy.get();
}

gx::Glyph& gx::Font::newGlyph(
  int code, int width, int height, float left, float top, float advX, float advY)
{
  Glyph& g = _glyphs[code];
  g.width = width;
  g.height = height;
  g.left = left;
  g.top = top;
  g.advX = advX;
  g.advY = advY;

  if ((code > 47 && code < 94) || (code > 96 && code < 127)) {
    // ymin/ymax adjust for a limited range of characters
    int yt = top;
    if (yt > _ymax) { _ymax = yt; }
    int yb = top - height;
    if (yb < _ymin) { _ymin = yb; }
  }

  return g;
}

float gx::Font::calcWidth(std::string_view text) const
{
  float max_width = 0, width = 0;
  const Glyph* g = nullptr;
  for (auto ch : text) {
    if (ch == '\n') {
      if (g) { width += -g->advX + (float(g->width) + g->left); }
      max_width = std::max(max_width, width);
      width = 0;
    }
    g = findGlyph(ch);
    if (g) { width += g->advX; }
  }

  if (g) { width += -g->advX + (float(g->width) + g->left); }
  return std::max(max_width, width);
}

int gx::Font::calcLines(std::string_view text) const
{
  if (text.empty()) { return 0; }
  int lines = 1;
  for (int ch : text) {
    if (ch == '\n') { ++lines; }
  }

  return lines;
}
