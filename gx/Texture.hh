//
// gx/Texture.hh
// Copyright (C) 2023 Richard Bradley
//

#pragma once
#include "Types.hh"
#include <utility>

namespace gx {
  class Texture;
  class Renderer;
  class Image;
}

class gx::Texture
{
 public:
  Texture() = default;

  template<class T>
  Texture(T& ren, const Image& img, int levels = 1,
          FilterType minFilter = FILTER_NEAREST,
          FilterType magFilter = FILTER_NEAREST) {
    init(ren, img, levels, minFilter, magFilter); }
  ~Texture() { cleanup(); }

  Texture(const Texture&) = delete;
  Texture& operator=(const Texture&) = delete;

  inline Texture(Texture&& t) noexcept;
  inline Texture& operator=(Texture&& t) noexcept;

  [[nodiscard]] explicit operator bool() const { return _texID != 0; }

  bool init(Renderer& ren, const Image& img, int levels = 1,
            FilterType minFilter = FILTER_NEAREST,
            FilterType magFilter = FILTER_NEAREST);

  template<class T>
  bool init(T& win, const Image& img, int levels = 1,
            FilterType minFilter = FILTER_NEAREST,
            FilterType magFilter = FILTER_NEAREST) {
    return init(win.renderer(), img, levels, minFilter, magFilter); }

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


// **** Inline Implementations ****
gx::Texture::Texture(Texture&& t) noexcept
  : _texID{std::exchange(t._texID,0)},
    _width{t._width}, _height{t._height}, _levels{t._levels},
    _minFilter{t._minFilter}, _magFilter{t._magFilter}
{ }

gx::Texture& gx::Texture::operator=(Texture&& t) noexcept
{
  cleanup();
  _texID = std::exchange(t._texID,0);
  _width = t._width;
  _height = t._height;
  _levels = t._levels;
  _minFilter = t._minFilter;
  _magFilter = t._magFilter;
  return *this;
}
