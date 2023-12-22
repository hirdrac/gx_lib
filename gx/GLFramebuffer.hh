//
// gx/GLFramebuffer.hh
// Copyright(C) 2023 by Richard Bradley
//
// wrapper for OpenGL framebuffer object
//

// NOTES:
// - attaching textures:
//   * 3.3: glFramebufferTexture2D
//   * 4.5: glNamedFramebufferTexture
// - renderbuffer (vs. textures)
//   * use for depth/stencil attachment if reading isn't necessary
//   * use for color attachment if final buffer (and reading isn't necessary)

#pragma once
#include "OpenGL.hh"
#include <utility>

namespace gx {
  template<int VER> class GLFramebuffer;
}

template<int VER>
class gx::GLFramebuffer
{
 public:
  using type = GLFramebuffer<VER>;

  GLFramebuffer() = default;
  ~GLFramebuffer() { if (GLInitialized) cleanup(); }

  // prevent copy/assignment
  GLFramebuffer(const type&) = delete;
  GLFramebuffer& operator=(const type&) = delete;

  // enable move
  inline GLFramebuffer(type&& fb) noexcept;
  inline GLFramebuffer& operator=(type&& fb) noexcept;

  // operators
  [[nodiscard]] explicit operator bool() const { return _fbuffer; }

  // accessors
  [[nodiscard]] GLuint id() const { return _fbuffer; }

  // methods
  inline GLuint init();
  GLuint release() noexcept { return std::exchange(_fbuffer, 0); }

  inline void bind();
  inline static void unbind();

  [[nodiscard]] inline GLenum status() const;

  void attachTexture(GLenum attachment, GLuint texture, GLint level);

 private:
  GLuint _fbuffer = 0;

  void bindCheck() {
    if (GLLastFramebufferBind != _fbuffer) { bind(); }
  }

  inline void cleanup() noexcept;
};


// **** Inline Implementations ****
template<int VER>
gx::GLFramebuffer<VER>::GLFramebuffer(gx::GLFramebuffer<VER>&& fb) noexcept
  : _fbuffer{fb.release()} { }

template<int VER>
gx::GLFramebuffer<VER>& gx::GLFramebuffer<VER>::operator=(
  gx::GLFramebuffer<VER>&& fb) noexcept
{
  if (this != &fb) {
    cleanup();
    _fbuffer = fb.release();
  }
  return *this;
}

template<int VER>
GLuint gx::GLFramebuffer<VER>::init()
{
  cleanup();
  if constexpr (VER < 45) {
    GX_GLCALL(glGenFramebuffers, 1, &_fbuffer);
  } else {
    GX_GLCALL(glCreateFramebuffers, 1, &_fbuffer);
  }
  return _fbuffer;
}

template<int VER>
void gx::GLFramebuffer<VER>::bind()
{
  GX_GLCALL(glBindFramebuffer, GL_FRAMEBUFFER, _fbuffer);
  if constexpr (VER < 45) { GLLastFramebufferBind = _fbuffer; }
}

template<int VER>
void gx::GLFramebuffer<VER>::unbind()
{
  GX_GLCALL(glBindFramebuffer, GL_FRAMEBUFFER, 0);
  if constexpr (VER < 45) { GLLastFramebufferBind = 0; }
}

template<int VER>
GLenum gx::GLFramebuffer<VER>::status() const
{
  if constexpr (VER < 45) {
    bindCheck();
    GLenum result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
#ifdef GX_DEBUG_GL
    if (result == 0) { GLCheckErrors("glCheckFramebufferStatus"); }
#endif
    return result;
  } else {
    GLenum result = glCheckNamedFramebufferStatus(_fbuffer, GL_FRAMEBUFFER);
#ifdef GX_DEBUG_GL
    if (result == 0) { GLCheckErrors("glCheckNamedFramebufferStatus"); }
#endif
    return result;
  }

  // success return value:
  // GL_FRAMEBUFFER_COMPLETE
  //
  // possible error return values:
  // GL_FRAMEBUFFER_UNDEFINED
  // GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT
  // GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT
  // GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER
  // GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER
  // GL_FRAMEBUFFER_UNSUPPORTED
  // GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE
  // GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS
}

template<int VER>
void gx::GLFramebuffer<VER>::attachTexture(
  GLenum attachment, GLuint texture, GLint level)
{
  // attachment values:
  // GL_COLOR_ATTACHMENTi (where i is 0 to GL_MAX_COLOR_ATTACHMENTS-1)
  // GL_DEPTH_ATTACHMENT
  // GL_STENCIL_ATTACHMENT
  // GL_DEPTH_STENCIL_ATTACHMENT

  if constexpr (VER < 45) {
    bindCheck();
    GX_GLCALL(glFramebufferTexture, GL_FRAMEBUFFER, attachment, texture, level);
  } else {
    GX_GLCALL(glNamedFramebufferTexture, _fbuffer, attachment, texture, level);
  }
}

template<int VER>
void gx::GLFramebuffer<VER>::cleanup() noexcept
{
  if (_fbuffer) {
    if constexpr (VER < 45) {
      if (GLLastFramebufferBind == _fbuffer) { GLLastFramebufferBind = 0; }
    }
    GX_GLCALL(glDeleteFramebuffers, 1, &_fbuffer);
  }
}
