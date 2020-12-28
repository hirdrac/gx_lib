//
// gx/Image.hh
// Copyright (C) 2020 Richard Bradley
//
// TODO - save to file
// TODO - load from static data

#pragma once
#include "Types.hh"
#include <string>
#include <memory>
#include <cstdint>
#include <cstring>


namespace gx {
  class Image;
  struct Glyph;
}

class gx::Image
{
 public:
  Image() = default;
  Image(Image&&) noexcept = default;
  Image& operator=(Image&&) noexcept = default;

  bool init(int width, int height, int channels);
  bool init(int width, int height, int channels,
	    const uint8_t* data, bool copy);
  bool init(const char* fileName);
  bool init(const std::string& fileName) { return init(fileName.c_str()); }

  [[nodiscard]] int width() const { return _width; }
  [[nodiscard]] int height() const { return _height; }
  [[nodiscard]] int channels() const { return _channels; }
  [[nodiscard]] std::size_t size() const { return _width * _height * _channels; }
  [[nodiscard]] const uint8_t* data() const { return _data; }

  // image editing methods
  bool canEdit() const { return _storage.get(); }
  bool clear();
  bool setPixel(int x, int y, const uint8_t* channels_vals);
  bool stamp(int x, int y, const Image& sub_image);
  bool stamp(int x, int y, const Glyph& g);

 private:
  const uint8_t* _data;
  std::unique_ptr<uint8_t[]> _storage;
  int _width, _height, _channels;

  Image(const Image&) = delete;
  Image& operator=(const Image&) = delete;
};
