//
// gx/Image.hh
// Copyright (C) 2026 Richard Bradley
//
// 8-bits per channel variable channel image
//

#pragma once
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
  Image(int width, int height, int channels) {
    init(width, height, channels); }
  Image(int width, int height, int channels, const uint8_t* data, bool copy) {
    init(width, height, channels, data, copy); }

  // allow copy/assign
  Image(const Image& im);
  Image& operator=(const Image& im);

  // enable move/move-assign
  Image(Image&&) noexcept = default;
  Image& operator=(Image&&) noexcept = default;

  // init methods
  void init(int width, int height, int channels);
  void init(int width, int height, int channels,
	    const uint8_t* data, bool copy);

  bool load(const char* fileName);

  template<class T>
  bool load(const T& fileName) { return load(fileName.c_str()); }

  bool loadFromMemory(const void* mem, std::size_t memSize);

  // accessors
  [[nodiscard]] explicit operator bool() const { return _data != nullptr; }
  [[nodiscard]] int width() const { return _width; }
  [[nodiscard]] int height() const { return _height; }
  [[nodiscard]] int channels() const { return _channels; }
  [[nodiscard]] std::size_t pixels() const {
    return std::size_t(_width * _height); }
  [[nodiscard]] std::size_t size() const {
    return pixels() * std::size_t(_channels); }
  [[nodiscard]] const uint8_t* data() const { return _data; }

  // image editing methods
  [[nodiscard]] bool canEdit() const { return _storage.get(); }
  void clear();
  void clear(const uint8_t* channelVals);
  void plot(int x, int y, const uint8_t* channelVals);
  void rectangle(int x, int y, int w, int h, const uint8_t* channelVals);
  void stamp(int x, int y, const Image& subImage);
  void stamp(int x, int y, const Glyph& g);

 private:
  std::unique_ptr<uint8_t[]> _storage;
  const uint8_t* _data = nullptr;
  int _width = 0, _height = 0, _channels = 0;

  [[nodiscard]] bool _valid(int x, int y) const {
    return (x >= 0) && (x < _width) && (y >= 0) && (y < _height);
  }

  [[nodiscard]] uint8_t* _ptr(int x, int y) {
    return _storage.get() + (((y * _width) + x) * _channels);
  }

  void _plot(int x, int y, const uint8_t* channelVals) {
    std::memcpy(_ptr(x,y), channelVals, std::size_t(_channels));
  }

  void _plot1(int x, int y, uint8_t val) {
    *(_storage.get() + (_width * y) + x) = val;
  }

  void _plot2(int x, int y, uint16_t val) {
    *(reinterpret_cast<uint16_t*>(_storage.get()) + (_width * y) + x) = val;
  }

  void _plot4(int x, int y, uint32_t val) {
    *(reinterpret_cast<uint32_t*>(_storage.get()) + (_width * y) + x) = val;
  }
};
