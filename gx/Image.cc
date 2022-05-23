//
// gx/Image.cc
// Copyright (C) 2022 Richard Bradley
//

#include "Image.hh"
#include "Glyph.hh"
#include "Logger.hh"
#include "Assert.hh"
#include "3rd/stb_image.h"
#include <limits>
#include <cstring>


bool gx::Image::init(int width, int height, int channels)
{
  GX_ASSERT(width > 0 && height > 0 && channels > 0);

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
  GX_ASSERT(width > 0 && height > 0 && channels > 0);
  GX_ASSERT(src_data != nullptr);

  _width = width;
  _height = height;
  _channels = channels;
  if (copy) {
    const std::size_t s = size();
    _storage.reset(new uint8_t[s]);
    std::memcpy(_storage.get(), src_data, s);
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
  GX_ASSERT(mem != nullptr);
  GX_ASSERT(memSize <= std::numeric_limits<int>::max());

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
  GX_ASSERT(canEdit());
  std::memset(_storage.get(), 0, size());
  return true;
}

bool gx::Image::setPixel(int x, int y, const uint8_t* channel_vals)
{
  GX_ASSERT(canEdit());
  const int offset = ((y * _width) + x) * _channels;
  GX_ASSERT((offset >= 0) && (offset + _channels) <= int(size()));

  std::memcpy(_storage.get() + offset, channel_vals, std::size_t(_channels));
  return true;
}

bool gx::Image::stamp(int x, int y, const Image& sub_image)
{
  GX_ASSERT(canEdit());
  GX_ASSERT(_channels == sub_image.channels());

  GX_ASSERT(x >= 0 && sub_image.width() <= (_width - x));
  GX_ASSERT(y >= 0 && sub_image.height() <= (_height - y));
  // TODO: clip source image against destination image

  uint8_t* dst = &_storage[std::size_t(((y * _width) + x) * _channels)];
  const uint8_t* src = sub_image.data();
  const int dw = _width * _channels;
  const std::size_t sw = std::size_t(sub_image.width() * sub_image.channels());
  for (int sy = 0; sy < sub_image.height(); ++sy) {
    std::memcpy(dst, src, sw);
    dst += dw;
    src += sw;
  }
  return true;
}

bool gx::Image::stamp(int x, int y, const Glyph& g)
{
  GX_ASSERT(canEdit());
  GX_ASSERT(_channels == 1);

  GX_ASSERT(x >= 0 && g.width <= (_width - x));
  GX_ASSERT(y >= 0 && g.height <= (_height - y));
  // TODO: clip source image against destination image

  uint8_t* dst = &_storage[std::size_t((y * _width) + x)];
  const uint8_t* src = g.bitmap;
  for (int sy = 0; sy < g.height; ++sy) {
    std::memcpy(dst, src, g.width);
    dst += _width;
    src += g.width;
  }
  return true;
}
