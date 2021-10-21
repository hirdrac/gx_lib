//
// gx/Image.hh
// Copyright (C) 2021 Richard Bradley
//
// TODO - save to file

#pragma once
#include "Types.hh"
#include <string>
#include <memory>


namespace gx {
  class Image;
  struct Glyph;
}

class gx::Image
{
 public:
  Image() = default;

  // prevent copy but allow move
  Image(const Image&) = delete;
  Image& operator=(const Image&) = delete;
  Image(Image&&) noexcept = default;
  Image& operator=(Image&&) noexcept = default;


  bool init(int width, int height, int channels);
  bool init(int width, int height, int channels,
	    const uint8_t* data, bool copy);

  bool load(const char* fileName);
  bool load(const std::string& fileName) { return load(fileName.c_str()); }
  bool loadFromMemory(const void* mem, std::size_t memSize);

  [[nodiscard]] int width() const { return _width; }
  [[nodiscard]] int height() const { return _height; }
  [[nodiscard]] int channels() const { return _channels; }
  [[nodiscard]] std::size_t size() const {
    return std::size_t(_width * _height * _channels); }
  [[nodiscard]] const uint8_t* data() const { return _data; }

  // image editing methods
  [[nodiscard]] bool canEdit() const { return _storage.get(); }
  bool clear();
  bool setPixel(int x, int y, const uint8_t* channels_vals);
  bool stamp(int x, int y, const Image& sub_image);
  bool stamp(int x, int y, const Glyph& g);

 private:
  const uint8_t* _data;
  std::unique_ptr<uint8_t[]> _storage;
  int _width, _height, _channels;
};
