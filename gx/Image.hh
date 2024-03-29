//
// gx/Image.hh
// Copyright (C) 2023 Richard Bradley
//
// 8-bits per channel variable channel image
//

#pragma once
#include <memory>
#include <cstdint>


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

  template<class T>
  bool load(const T& fileName) { return load(fileName.c_str()); }

  bool loadFromMemory(const void* mem, std::size_t memSize);

  [[nodiscard]] int width() const { return _width; }
  [[nodiscard]] int height() const { return _height; }
  [[nodiscard]] int channels() const { return _channels; }
  [[nodiscard]] std::size_t size() const {
    return std::size_t(_width * _height * _channels); }
  [[nodiscard]] const uint8_t* data() const { return _data; }

  // image editing methods
  [[nodiscard]] bool canEdit() const { return _storage.get(); }
  void clear();
  void plot(int x, int y, const uint8_t* channels_vals);
  void stamp(int x, int y, const Image& sub_image);
  void stamp(int x, int y, const Glyph& g);

 private:
  const uint8_t* _data;
  std::unique_ptr<uint8_t[]> _storage;
  int _width, _height, _channels;
};
