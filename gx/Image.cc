//
// gx/Image.cc
// Copyright (C) 2021 Richard Bradley
//

#include "Image.hh"
#include "Glyph.hh"
#include "Logger.hh"
#include "3rd/stb_image.h"

#include <limits>
#include <cassert>


bool gx::Image::init(int width, int height, int channels)
{
  assert(width > 0 && height > 0 && channels > 0);

  _width = width;
  _height = height;
  _channels = channels;
  _storage = std::make_unique<uint8_t[]>(size());
  _data = _storage.get();
  return true;
}

bool gx::Image::init(int width, int height, int channels,
		     const uint8_t* src_data, bool copy)
{
  assert(width > 0 && height > 0 && channels > 0);
  assert(src_data != nullptr);

  _width = width;
  _height = height;
  _channels = channels;
  if (copy) {
    _storage.reset(new uint8_t[size()]);
    std::memcpy(_storage.get(), src_data, size());
    _data = _storage.get();
  } else {
    _data = src_data;
  }
  return true;
}

bool gx::Image::load(const char* filename)
{
  int w = 0, h = 0, c = 0;
  uint8_t* data = stbi_load(filename, &w, &h, &c, 0);
  if (!data) {
    GX_LOG_ERROR("stbi_load() failed");
    return false;
  }

  init(w, h, c, data, true);
  stbi_image_free(data);
  return true;
}

bool gx::Image::loadFromMemory(const void* mem, std::size_t memSize)
{
  assert(mem != nullptr);
  assert(memSize <= std::numeric_limits<int>::max());

  int w = 0, h = 0, c = 0;
  uint8_t* data = stbi_load_from_memory(
    static_cast<const stbi_uc*>(mem), int(memSize), &w, &h, &c, 0);
  if (!data) {
    GX_LOG_ERROR("stbi_load_from_memory() failed");
    return false;
  }

  init(w, h, c, data, true);
  stbi_image_free(data);
  return true;
}

bool gx::Image::clear()
{
  assert(canEdit());
  std::memset(_storage.get(), 0, size());
  return true;
}

bool gx::Image::setPixel(int x, int y, const uint8_t* channel_vals)
{
  assert(canEdit());
  int offset = ((y * _width) + x) * _channels;
  assert((offset >= 0) && (offset + _channels) <= int(size()));

  std::memcpy(_storage.get() + offset, channel_vals, std::size_t(_channels));
  return true;
}

bool gx::Image::stamp(int x, int y, const Image& sub_image)
{
  assert(canEdit());
  assert(_channels == sub_image.channels());
  // FIXME - check for out-of-bounds setting

  uint8_t* dst = _storage.get() + (((y * _width) + x) * _channels);
  const uint8_t* src = sub_image.data();
  int dw = _width * _channels;
  std::size_t sw = std::size_t(sub_image.width() * sub_image.channels());
  for (int sy = 0; sy < sub_image.height(); ++sy) {
    std::memcpy(dst, src, sw);
    dst += dw;
    src += sw;
  }
  return true;
}

bool gx::Image::stamp(int x, int y, const Glyph& g)
{
  assert(canEdit());
  assert(_channels == 1);

  uint8_t* dst = _storage.get() + (y * _width) + x;
  const uint8_t* src = g.bitmap;
  for (int sy = 0; sy < g.height; ++sy) {
    std::memcpy(dst, src, g.width);
    dst += _width;
    src += g.width;
  }
  return true;
}
