//
// gx/GLRenderbuffer.hh
// Copyright (C) 2023 by Richard Bradley
//
// wrapper for OpenGL renderbuffer object
//

#pragma once
#include "OpenGL.hh"

namespace gx {
  template<int VER> class GLRenderbuffer;
}

template<int VER>
class gx::GLRenderbuffer
{
 public:
  using type = GLRenderbuffer<VER>;

  GLRenderbuffer() = default;
  ~GLRenderbuffer() { if (GLInitialized) cleanup(); }

  // prevent copy/assignment
  GLRenderbuffer(const type&) = delete;
  GLRenderbuffer& operator=(const type&) = delete;

  // enable move
  inline GLRenderbuffer(type&& rb) noexcept;
  inline GLRenderbuffer& operator=(type&& rb) noexcept;

  // operators
  [[nodiscard]] explicit operator bool() const { return _rbuffer; }

  // accessors
  [[nodiscard]] GLuint id() const { return _rbuffer; }

  // methods
  inline GLuint init(GLenum internalformat, GLsizei width, GLsizei height);
  GLuint release() noexcept { return std::exchange(_rbuffer, 0); }

  inline void bind();
  inline static void unbind();

 private:
  GLuint _rbuffer = 0;

  void bindCheck() {
    if (GLLastRenderbufferBind != _rbuffer) { bind(); }
  }

  inline void cleanup() noexcept;
};


// **** Inline Implementations ****
template<int VER>
gx::GLRenderbuffer<VER>::GLRenderbuffer(gx::GLRenderbuffer<VER>&& rb) noexcept
  : _rbuffer{rb.release()} { }

template<int VER>
gx::GLRenderbuffer<VER>& gx::GLRenderbuffer<VER>::operator=(
  gx::GLRenderbuffer<VER>&& rb) noexcept
{
  if (this != &rb) {
    cleanup();
    _rbuffer = rb.release();
  }
  return *this;
}

template<int VER>
GLuint gx::GLRenderbuffer<VER>::init(
  GLenum internalformat, GLsizei width, GLsizei height)
{
  cleanup();
  if constexpr (VER < 45) {
    GX_GLCALL(glGenRenderbuffers, 1, &_rbuffer);
    bind();
    GX_GLCALL(glRenderbufferStorage, GL_RENDERBUFFER, internalformat, width, height);
    //glRenderbufferStorageMultisample
  } else {
    GX_GLCALL(glCreateRenderbuffers, 1, &_rbuffer);
    GX_GLCALL(glNamedRenderbufferStorage, _rbuffer, internalformat, width, height);
    //glNamedRenderbufferStorageMultisample
  }
  return _rbuffer;
}

template<int VER>
void gx::GLRenderbuffer<VER>::bind()
{
  GX_GLCALL(glBindRenderbuffer, GL_RENDERBUFFER, _rbuffer);
  if constexpr (VER < 45) { GLLastRenderbufferBind = _rbuffer; }
}

template<int VER>
void gx::GLRenderbuffer<VER>::unbind()
{
  GX_GLCALL(glBindRenderbuffer, GL_RENDERBUFFER, 0);
  if constexpr (VER < 45) { GLLastRenderbufferBind = 0; }
}

template<int VER>
void gx::GLRenderbuffer<VER>::cleanup() noexcept
{
  if (_rbuffer) {
    if constexpr (VER < 45) {
      if (GLLastRenderbufferBind == _rbuffer) { GLLastRenderbufferBind = 0; }
    }
    GX_GLCALL(glDeleteRenderbuffers, 1, &_rbuffer);
  }
}
