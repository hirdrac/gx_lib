//
// gx/Image.cc
// Copyright (C) 2026 Richard Bradley
//

#include "Image.hh"
#include "Logger.hh"
#include "Assert.hh"
#include "3rd/stb_image.h"
#include <limits>
using namespace gx;


Image::Image(const Image& im)
  : _data{im._data}, _width{im._width}, _height{im._height},
    _channels{im._channels}
{
  if (im.owner()) { _makeStorage(); }
}

Image& Image::operator=(const Image& im)
{
  GX_ASSERT(this != &im);

  if (im.owner()) {
    const std::size_t s = im.size();
    if (owner() && size() == s) {
      // reuse existing buffer
      std::memcpy(_storage.get(), im.data(), s);
    } else {
      // copy other buffer (strong exception safety)
      auto buf = std::make_unique_for_overwrite<uint8_t[]>(s);
      std::memcpy(buf.get(), im._data, s);
      _storage = std::move(buf);
      _data = _storage.get();
    }
  } else {
    _storage = {};
    _data = im._data;
  }
  _width = im._width;
  _height = im._height;
  _channels = im._channels;
  return *this;
}

void Image::init(int width, int height, int channels)
{
  GX_ASSERT(width > 0 && height > 0 && channels > 0 && channels <= 4);

  // strong exception safety
  auto buf = std::make_unique<uint8_t[]>(
    std::size_t(width * height * channels));

  _storage = std::move(buf);
  _data = _storage.get();
  _width = width;
  _height = height;
  _channels = channels;
}

void Image::init(int width, int height, int channels,
                 const uint8_t* data, bool copy)
{
  GX_ASSERT(width > 0 && height > 0 && channels > 0 && channels <= 4);
  GX_ASSERT(data != nullptr);

  if (copy) {
    // strong exception safety
    const auto s = std::size_t(width * height * channels);
    auto buf = std::make_unique_for_overwrite<uint8_t[]>(s);
    std::memcpy(buf.get(), data, s);

    _storage = std::move(buf);
    _data = _storage.get();
  } else {
    _storage = {};
    _data = data;
  }
  _width = width;
  _height = height;
  _channels = channels;
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
  if (!owner()) { _makeStorage(); }
  std::memset(_storage.get(), 0, size());
}

void Image::clear(const uint8_t* channelVals)
{
  if (!owner()) { _makeStorage(); }
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
  if (!owner()) { _makeStorage(); }
  if (_valid(x, y)) { _plot(x, y, channelVals); }
}

void Image::rectangle(int x, int y, int w, int h, const uint8_t* channelVals)
{
  if (!owner()) { _makeStorage(); }
  if ((x >= _width) || (y >= _height)
      || ((x + w) <= 0) || ((y + h) <= 0)) { return; }

  // clip rectangle to image boundries
  const int x0 = std::max(x, 0);
  const int y0 = std::max(y, 0);
  const int x1 = std::min(x + w - 1, _width - 1);
  const int y1 = std::min(y + h - 1, _height - 1);
  const int ww = x1 - x0 + 1;
  const int hh = y1 - y0 + 1;
  uint8_t* ptr = _ptr(x0, y0);

  switch (_channels) {
    default: {
      const uint8_t val = channelVals[0];
      uint8_t* dst = ptr;
      uint8_t* end = dst + (_width * hh);
      for (; dst != end; dst += _width) { std::fill_n(dst, ww, val); }
      break;
    }
    case 2: {
      const uint16_t val = *reinterpret_cast<const uint16_t*>(channelVals);
      uint16_t* dst = reinterpret_cast<uint16_t*>(ptr);
      uint16_t* end = dst + (_width * hh);
      for (; dst != end; dst += _width) { std::fill_n(dst, ww, val); }
      break;
    }
    case 3: {
      const uint8_t v0 = channelVals[0];
      const uint8_t v1 = channelVals[1];
      const uint8_t v2 = channelVals[2];
      const int next = _width * 3;
      uint8_t* row = ptr;
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
      uint32_t* dst = reinterpret_cast<uint32_t*>(ptr);
      uint32_t* end = dst + (_width * hh);
      for (; dst != end; dst += _width) { std::fill_n(dst, ww, val); }
      break;
    }
  }
}

void Image::stamp(int x, int y, const Image& subImage)
{
  GX_ASSERT(_channels == subImage.channels());

  if (x >= _width || y >= _height) { return; }

  int subX = 0, subWidth = subImage.width();
  if (x < 0) {
    subX = -x;
    if (subX >= subImage.width()) { return; }
    x = 0;
    subWidth -= subX;
  }

  int subY = 0, subHeight = subImage.height();
  if (y < 0) {
    subY = -y;
    if (subY >= subImage.height()) { return; }
    y = 0;
    subHeight -= subY;
  }

  subWidth = std::min(subWidth, _width - x);
  subHeight = std::min(subHeight, _height - y);

  if (!owner()) { _makeStorage(); }
  const int dstRow = _width * _channels;
  uint8_t* dst = _ptr(x,y);

  const auto sw = std::size_t(subWidth * subImage.channels());
  const int srcRow = subImage.width() * subImage.channels();
  const uint8_t* src = subImage._ptr(subX, subY);
  const uint8_t* srcEnd = src + (srcRow * subHeight);

  for (; src != srcEnd; src += srcRow) {
    std::memcpy(dst, src, sw);
    dst += dstRow;
  }
}
