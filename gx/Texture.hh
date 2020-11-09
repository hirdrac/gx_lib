//
// gx/Texture.hh
// Copyright (C) 2020 Richard Bradley
//

#pragma once
#include "types.hh"
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
  Texture(Window& win, const Image& img) { init(win,img); }
  ~Texture() { cleanup(); }

  Texture(const Texture&) = delete;
  Texture& operator=(const Texture&) = delete;

  Texture(Texture&& t) noexcept : _texID(std::exchange(t._texID,0)) { }
  Texture& operator=(Texture&& t) noexcept;

  explicit operator bool() const { return _texID != 0; }

  bool init(Window& win, const Image& img);
  bool update(const Image& img);

  [[nodiscard]] int id() const { return _texID; }

 private:
  std::shared_ptr<Renderer> _renderer;
  int _texID = 0;

  void cleanup();
};
