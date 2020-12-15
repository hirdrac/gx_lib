//
// gx/Texture.hh
// Copyright (C) 2020 Richard Bradley
//

#pragma once
#include "Types.hh"
#include <utility>
#include <memory>

namespace gx {
  class Texture;
  class Window;
  class Renderer;
  class Image;
}

class gx::Texture
{
 public:
  Texture();
  Texture(Window& win, const Image& img,
          FilterType minFilter = FILTER_NEAREST,
          FilterType magFilter = FILTER_NEAREST) {
    init(win, img, minFilter, magFilter); }
  ~Texture() { cleanup(); }

  Texture(const Texture&) = delete;
  Texture& operator=(const Texture&) = delete;

  Texture(Texture&& t) noexcept
    : _texID(std::exchange(t._texID,0)),
      _minFilter(t._minFilter), _magFilter(t._magFilter) { }
  Texture& operator=(Texture&& t) noexcept;

  explicit operator bool() const { return _texID != 0; }

  bool init(Window& win, const Image& img,
            FilterType minFilter = FILTER_NEAREST,
            FilterType magFilter = FILTER_NEAREST);
  bool update(const Image& img);

  [[nodiscard]] int id() const { return _texID; }
  [[nodiscard]] FilterType minFilter() const { return _minFilter; }
  [[nodiscard]] FilterType magFilter() const { return _magFilter; }

 private:
  std::shared_ptr<Renderer> _renderer;
  int _texID = 0;
  FilterType _minFilter = FILTER_NEAREST;
  FilterType _magFilter = FILTER_NEAREST;

  void cleanup();
};
