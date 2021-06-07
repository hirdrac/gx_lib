//
// gx/Texture.hh
// Copyright (C) 2021 Richard Bradley
//

#pragma once
#include "Types.hh"
#include <utility>  // for std::exchange()

namespace gx {
  class Texture;
  class Renderer;
  class Image;
}

class gx::Texture
{
 public:
  Texture();
  Texture(Renderer& ren, const Image& img, int levels = 1,
          FilterType minFilter = FILTER_NEAREST,
          FilterType magFilter = FILTER_NEAREST) {
    init(ren, img, levels, minFilter, magFilter); }
  ~Texture() { cleanup(); }

  Texture(const Texture&) = delete;
  Texture& operator=(const Texture&) = delete;

  Texture(Texture&& t) noexcept
    : _texID(std::exchange(t._texID,0)),
      _minFilter(t._minFilter), _magFilter(t._magFilter) { }
  Texture& operator=(Texture&& t) noexcept;

  [[nodiscard]] explicit operator bool() const { return _texID != 0; }

  bool init(Renderer& ren, const Image& img, int levels = 1,
            FilterType minFilter = FILTER_NEAREST,
            FilterType magFilter = FILTER_NEAREST);
  bool update(const Image& img);

  [[nodiscard]] TextureID id() const { return _texID; }
  [[nodiscard]] int width() const { return _width; }
  [[nodiscard]] int height() const { return _height; }
  [[nodiscard]] int levels() const { return _levels; }
  [[nodiscard]] FilterType minFilter() const { return _minFilter; }
  [[nodiscard]] FilterType magFilter() const { return _magFilter; }

 private:
  TextureID _texID = 0;
  int _width = 0, _height = 0;
  int _levels = 1; // mipmap levels
  FilterType _minFilter = FILTER_NEAREST;
  FilterType _magFilter = FILTER_NEAREST;

  void cleanup();
};
