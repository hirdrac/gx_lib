//
// gx/Texture.hh
// Copyright (C) 2021 Richard Bradley
//

#pragma once
#include "RendererPtr.hh"
#include "Types.hh"
#include <utility>

namespace gx {
  class Texture;
  class Window;
  class Image;
}

class gx::Texture
{
 public:
  Texture();
  Texture(Window& win, const Image& img, int levels = 1,
          FilterType minFilter = FILTER_NEAREST,
          FilterType magFilter = FILTER_NEAREST) {
    init(win, img, levels, minFilter, magFilter); }
  ~Texture() { cleanup(); }

  Texture(const Texture&) = delete;
  Texture& operator=(const Texture&) = delete;

  Texture(Texture&& t) noexcept
    : _texID(std::exchange(t._texID,0)),
      _minFilter(t._minFilter), _magFilter(t._magFilter) { }
  Texture& operator=(Texture&& t) noexcept;

  [[nodiscard]] explicit operator bool() const { return _texID != 0; }

  bool init(Window& win, const Image& img, int levels = 1,
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
  RendererPtr _renderer;
  TextureID _texID = 0;
  int _width = 0, _height = 0;
  int _levels = 1; // mipmap levels
  FilterType _minFilter = FILTER_NEAREST;
  FilterType _magFilter = FILTER_NEAREST;

  void cleanup();
};
