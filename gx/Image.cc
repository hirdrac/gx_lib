//
// gx/Image.cc
// Copyright (C) 2026 Richard Bradley
//

#include "Image.hh"
#include "Glyph.hh"
#include "Logger.hh"
#include "Assert.hh"
#include "3rd/stb_image.h"
#include <limits>
using namespace gx;


Image::Image(const Image& im)
  : _width{im._width}, _height{im._height}, _channels{im._channels}
{
  const std::size_t s = im.size();
  if (s > 0) {
    _storage = std::make_unique_for_overwrite<uint8_t[]>(s);
    _data = _storage.get();
    std::memcpy(_storage.get(), im.data(), s);
  }
}

Image& Image::operator=(const Image& im)
{
  const std::size_t s = im.size();
  if (s > 0) {
    if (!_storage.get() || size() != s) {
      _storage = std::make_unique_for_overwrite<uint8_t[]>(s);
      _data = _storage.get();
    }
    std::memcpy(_storage.get(), im.data(), s);
  } else {
    _storage = {};
    _data = nullptr;
  }
  _width = im._width;
  _height = im._height;
  _channels = im._channels;
  return *this;
}

void Image::init(int width, int height, int channels)
{
  GX_ASSERT(width > 0 && height > 0 && channels > 0 && channels <= 4);

  _width = width;
  _height = height;
  _channels = channels;
  _storage = std::make_unique<uint8_t[]>(size());
  _data = _storage.get();
}

void Image::init(int width, int height, int channels,
                 const uint8_t* src_data, bool copy)
{
  GX_ASSERT(width > 0 && height > 0 && channels > 0 && channels <= 4);
  GX_ASSERT(src_data != nullptr);

  _width = width;
  _height = height;
  _channels = channels;
  if (copy) {
    const std::size_t s = size();
    _storage = std::make_unique_for_overwrite<uint8_t[]>(s);
    _data = _storage.get();
    std::memcpy(_storage.get(), src_data, s);
  } else {
    _storage = {};
    _data = src_data;
  }
}

bool Image::load(const char* filename)
{
  GX_ASSERT(filename != nullptr);

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

bool Image::loadFromMemory(const void* mem, std::size_t memSize)
{
  GX_ASSERT(mem != nullptr);
  GX_ASSERT(memSize > 0 && memSize <= std::numeric_limits<int>::max());

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

void Image::clear()
{
  GX_ASSERT(canEdit());
  std::memset(_storage.get(), 0, size());
}

void Image::clear(const uint8_t* channelVals)
{
  GX_ASSERT(canEdit());
  switch (_channels) {
    default:
      std::memset(_storage.get(), channelVals[0], size());
      break;
    case 2:
      std::fill_n(reinterpret_cast<uint16_t*>(_storage.get()), pixels(),
                  *reinterpret_cast<const uint16_t*>(channelVals));
      break;
    case 3: {
      const uint8_t v0 = channelVals[0];
      const uint8_t v1 = channelVals[1];
      const uint8_t v2 = channelVals[2];
      uint8_t* dst = _storage.get();
      uint8_t* end = dst + size();
      while (dst != end) { *dst++ = v0; *dst++ = v1; *dst++ = v2; }
      break;
    }
    case 4:
      std::fill_n(reinterpret_cast<uint32_t*>(_storage.get()), pixels(),
                  *reinterpret_cast<const uint32_t*>(channelVals));
      break;
  }
}

void Image::plot(int x, int y, const uint8_t* channelVals)
{
  GX_ASSERT(canEdit());
  if (_valid(x, y)) { _plot(x, y, channelVals); }
}

void Image::rectangle(int x, int y, int w, int h, const uint8_t* channelVals)
{
  GX_ASSERT(canEdit());
  if ((x >= _width) || (y >= _height)
      || ((x+_width) <= 0) || ((y+_height) <= 0)) { return; }

  // clip rectangle to image boundries
  const int x0 = std::max(x, 0);
  const int y0 = std::max(y, 0);
  const int x1 = std::min(x + w - 1, _width - 1);
  const int y1 = std::min(y + h - 1, _height - 1);
  const int ww = x1 - x0 + 1;
  const int hh = y1 - y0 + 1;

  switch (_channels) {
    default: {
      const uint8_t val = channelVals[0];
      uint8_t* dst = _ptr(x, y);
      uint8_t* end = dst + (_width * hh);
      for (; dst != end; dst += _width) { std::fill_n(dst, ww, val); }
      break;
    }
    case 2: {
      const uint16_t val = *reinterpret_cast<const uint16_t*>(channelVals);
      uint16_t* dst = reinterpret_cast<uint16_t*>(_ptr(x,y));
      uint16_t* end = dst + (_width * hh);
      for (; dst != end; dst += _width) { std::fill_n(dst, ww, val); }
      break;
    }
    case 3: {
      const uint8_t v0 = channelVals[0];
      const uint8_t v1 = channelVals[1];
      const uint8_t v2 = channelVals[2];
      const int next = _width * 3;
      uint8_t* row = _ptr(x,y);
      uint8_t* rowEnd = row + (next * hh);
      for (; row != rowEnd; row += next) {
        uint8_t* dst = row;
        uint8_t* end = row + (ww * 3);
        while (dst != end) { *dst++ = v0; *dst++ = v1; *dst++ = v2; }
      }
      break;
    }
    case 4: {
      const uint32_t val = *reinterpret_cast<const uint32_t*>(channelVals);
      uint32_t* dst = reinterpret_cast<uint32_t*>(_ptr(x,y));
      uint32_t* end = dst + (_width * hh);
      for (; dst != end; dst += _width) { std::fill_n(dst, ww, val); }
      break;
    }
  }
}

void Image::stamp(int x, int y, const Image& subImage)
{
  GX_ASSERT(canEdit());
  GX_ASSERT(_channels == subImage.channels());

  GX_ASSERT(x >= 0 && subImage.width() <= (_width - x));
  GX_ASSERT(y >= 0 && subImage.height() <= (_height - y));
  // TODO: clip source image against destination image

  uint8_t* dst = _ptr(x,y);
  const uint8_t* src = subImage.data();
  const int dw = _width * _channels;
  const std::size_t sw = std::size_t(subImage.width() * subImage.channels());
  for (int sy = 0; sy < subImage.height(); ++sy) {
    std::memcpy(dst, src, sw);
    dst += dw;
    src += sw;
  }
}

void Image::stamp(int x, int y, const Glyph& g)
{
  GX_ASSERT(canEdit());
  GX_ASSERT(_channels == 1);

  GX_ASSERT(x >= 0 && g.width <= (_width - x));
  GX_ASSERT(y >= 0 && g.height <= (_height - y));
  // TODO: clip source image against destination image

  uint8_t* dst = _ptr(x,y);
  const uint8_t* src = g.bitmap;
  for (int sy = 0; sy < g.height; ++sy) {
    std::memcpy(dst, src, g.width);
    dst += _width;
    src += g.width;
  }
}
