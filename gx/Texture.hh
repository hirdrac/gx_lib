//
// gx/Texture.hh
// Copyright (C) 2024 Richard Bradley
//
// Handle for Texture created in Renderer
//

#pragma once
#include "Renderer.hh"
#include <utility>

namespace gx {
  class Texture;
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

  [[nodiscard]] TextureID id() const { return _texID; }

 private:
  TextureID _texID = 0;

  void cleanup();
};


// **** Inline Implementations ****
gx::Texture::Texture(Texture&& t) noexcept
  : _texID{std::exchange(t._texID,0)}
{ }

gx::Texture& gx::Texture::operator=(Texture&& t) noexcept
{
  cleanup();
  _texID = std::exchange(t._texID,0);
  return *this;
}
