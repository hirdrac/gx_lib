//
// gx/GLTexture.hh
// Copyright (C) 2023 Richard Bradley
//
// wrapper for OpenGL texture object
// (texture target as a template parameter)
//

// FIXME: GL_TEXTURE_CUBE_MAP/GL_TEXTURE_CUBE_MAP_ARRAY broken
//   see https://www.khronos.org/opengl/wiki/Cubemap_Texture

#pragma once
#include "OpenGL.hh"
#include <utility>
#include <memory>

namespace gx {
  template<int VER>                class GLTexture1D;
  template<int VER, GLenum TARGET> class GLTexture2DT;
  template<int VER, GLenum TARGET> class GLTexture3DT;
  template<int VER>                class GLTextureBuffer;


  // **** Helper type aliases ****
  template<int VER>
  using GLTexture2D = GLTexture2DT<VER,GL_TEXTURE_2D>;

  template<int VER>
  using GLTexture1DArray = GLTexture2DT<VER,GL_TEXTURE_1D_ARRAY>;

  template<int VER>
  using GLTextureRectangle = GLTexture2DT<VER,GL_TEXTURE_RECTANGLE>;

  template<int VER>
  using GLTextureCubeMap = GLTexture2DT<VER,GL_TEXTURE_CUBE_MAP>;

  template<int VER>
  using GLTexture2DMultisample = GLTexture2DT<VER,GL_TEXTURE_2D_MULTISAMPLE>;

  template<int VER>
  using GLTexture3D = GLTexture3DT<VER,GL_TEXTURE_3D>;

  template<int VER>
  using GLTexture2DArray = GLTexture3DT<VER,GL_TEXTURE_2D_ARRAY>;

  template<int VER>
  using GLTextureCubeMapArray = GLTexture3DT<VER,GL_TEXTURE_CUBE_MAP_ARRAY>;

  template<int VER>
  using GLTexture2DMultisampleArray =
    GLTexture3DT<VER,GL_TEXTURE_2D_MULTISAMPLE_ARRAY>;


  // **** Texture constants based on target ****
  template<GLenum TARGET> struct GLTextureVals {
    static constexpr GLenum pnameMaxSize = GL_MAX_TEXTURE_SIZE;
  };

  template<> struct GLTextureVals<GL_TEXTURE_3D> {
    static constexpr GLenum pnameMaxSize = GL_MAX_3D_TEXTURE_SIZE;
  };

  template<> struct GLTextureVals<GL_TEXTURE_RECTANGLE> {
    static constexpr GLenum pnameMaxSize = GL_MAX_RECTANGLE_TEXTURE_SIZE;
  };

  template<> struct GLTextureVals<GL_TEXTURE_CUBE_MAP> {
    static constexpr GLenum pnameMaxSize = GL_MAX_CUBE_MAP_TEXTURE_SIZE;
  };

  template<> struct GLTextureVals<GL_TEXTURE_CUBE_MAP_ARRAY> {
    static constexpr GLenum pnameMaxSize = GL_MAX_CUBE_MAP_TEXTURE_SIZE;
  };

  template<> struct GLTextureVals<GL_TEXTURE_BUFFER> {
    static constexpr GLenum pnameMaxSize = GL_MAX_TEXTURE_BUFFER_SIZE;
  };
}

// **** Main texture template definition ****
template<int VER>
class gx::GLTexture1D
{
 public:
  using type = GLTexture1D<VER>;

  GLTexture1D() = default;
  ~GLTexture1D() { if (GLInitialized) cleanup(); }

  // prevent copy/assignment
  GLTexture1D(const type&) = delete;
  type& operator=(const type&) = delete;

  // allow move/move-assign
  inline GLTexture1D(type&& t) noexcept;
  inline type& operator=(type&& t) noexcept;

  // operators
  [[nodiscard]] explicit operator bool() const { return _tex; }

  // accessors
  [[nodiscard]] static GLenum target() { return GL_TEXTURE_1D; }
  [[nodiscard]] GLuint id() const { return _tex; }
  [[nodiscard]] GLenum internalFormat() const { return _internalformat; }
  [[nodiscard]] GLsizei levels() const { return _levels; }
  [[nodiscard]] GLsizei width() const { return _width; }

  // methods
  inline GLuint init(GLsizei levels, GLenum internalformat, GLsizei width);

  GLuint release() noexcept { return std::exchange(_tex, 0); }
    // releases ownership of managed texture object, returns object id

  inline static void bindUnit(GLuint unit, GLuint tex);
  void bindUnit(GLuint unit) const { bindUnit(unit, _tex); }
  static void unbindUnit(GLuint unit) { bindUnit(unit, 0); }

  inline void setSubImage1D(
    GLint level, GLint xoffset, GLsizei width,
    GLenum format, GLenum type, const void* pixels);

  template<typename PixelT>
  void setSubImage1D(GLint level, GLint xoffset, GLsizei width,
                     GLenum format, const PixelT* pixels) {
    setSubImage1D(level, xoffset, width, format, GLType_v<PixelT>, pixels); }

  inline void getImage(
    GLint level, GLenum format, GLenum type, GLsizei bufSize, void* pixels);

  inline void generateMipmap();
  inline void clear(GLint level);

  // set texture parameters
  inline void setParameter(GLenum pname, GLfloat param);
  inline void setParameter(GLenum pname, GLint param);
  inline void setParameterv(GLenum pname, const GLfloat* params);
  inline void setParameterv(GLenum pname, const GLint* params);
  inline void setParameterIv(GLenum pname, const GLint* params);
  inline void setParameterIv(GLenum pname, const GLuint* params);

  [[nodiscard]] static GLint maxSize() {
    GLint s = 0;
    GX_GLCALL(glGetIntegerv, GLTextureVals<target()>::pnameMaxSize, &s);
    return s;
  }

 private:
  GLuint _tex = 0;
  GLenum _internalformat = GL_NONE;
  GLsizei _levels = 0;
  GLsizei _width = 0;

  void bindCheck() {
    if (GLLastTextureBind != _tex) {
      // don't care which unit is active
      GX_GLCALL(glBindTexture, target(), _tex);
      GLLastTextureBind = _tex;
    }
  }

  void cleanup() noexcept {
    if (_tex) {
      if constexpr (VER < 45) {
        if (GLLastTextureBind == _tex) { GLLastTextureBind = 0; }
      }
      GX_GLCALL(glDeleteTextures, 1, &_tex);
    }
  }
};


template<int VER, GLenum TARGET>
class gx::GLTexture2DT
{
 public:
  using type = GLTexture2DT<VER,TARGET>;

  GLTexture2DT() = default;
  ~GLTexture2DT() { if (GLInitialized) cleanup(); }

  // prevent copy/assignment
  GLTexture2DT(const type&) = delete;
  type& operator=(const type&) = delete;

  // allow move/move-assign
  inline GLTexture2DT(type&& t) noexcept;
  inline type& operator=(type&& t) noexcept;

  // operators
  [[nodiscard]] explicit operator bool() const { return _tex; }

  // accessors
  [[nodiscard]] static GLenum target() { return TARGET; }
  [[nodiscard]] GLuint id() const { return _tex; }
  [[nodiscard]] GLenum internalFormat() const { return _internalformat; }
  [[nodiscard]] GLsizei levels() const { return _levels; }
  [[nodiscard]] GLsizei width() const { return _width; }
  [[nodiscard]] GLsizei height() const { return _height; }

  // methods
  inline GLuint init(GLsizei levels, GLenum internalformat,
		     GLsizei width, GLsizei height);
    // for GL_TEXTURE_2D, GL_TEXTURE_1D_ARRAY, GL_TEXTURE_RECTANGLE,
    //   GL_TEXTURE_CUBE_MAP

  GLuint release() noexcept { return std::exchange(_tex, 0); }
    // releases ownership of managed texture object, returns object id

  inline static void bindUnit(GLuint unit, GLuint tex);
  void bindUnit(GLuint unit) const { bindUnit(unit, _tex); }
  static void unbindUnit(GLuint unit) { bindUnit(unit, 0); }

  inline void setSubImage2D(
    GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
    GLenum format, GLenum type, const void* pixels);

  template<typename PixelT>
  void setSubImage2D(GLint level, GLint xoffset, GLint yoffset, GLsizei width,
                     GLsizei height, GLenum format, const PixelT* pixels) {
    setSubImage2D(level, xoffset, yoffset, width, height, format,
                  GLType_v<PixelT>, pixels); }

  inline void getImage(
    GLint level, GLenum format, GLenum type, GLsizei bufSize, void* pixels);

  inline void generateMipmap();
    // not valid for GL_TEXTURE_RECTANGLE, GL_TEXTURE_2D_MULTISAMPLE

  inline void clear(GLint level);

  // set texture parameters
  inline void setParameter(GLenum pname, GLfloat param);
  inline void setParameter(GLenum pname, GLint param);
  inline void setParameterv(GLenum pname, const GLfloat* params);
  inline void setParameterv(GLenum pname, const GLint* params);
  inline void setParameterIv(GLenum pname, const GLint* params);
  inline void setParameterIv(GLenum pname, const GLuint* params);

  [[nodiscard]] static GLint maxSize() {
    GLint s = 0;
    GX_GLCALL(glGetIntegerv, GLTextureVals<TARGET>::pnameMaxSize, &s);
    return s;
  }

 private:
  GLuint _tex = 0;
  GLenum _internalformat = GL_NONE;
  GLsizei _levels = 0;
  GLsizei _width = 0;
  GLsizei _height = 0;

  void bindCheck() {
    if (GLLastTextureBind != _tex) {
      // don't care which unit is active
      GX_GLCALL(glBindTexture, TARGET, _tex);
      GLLastTextureBind = _tex;
    }
  }

  void cleanup() noexcept {
    if (_tex) {
      if constexpr (VER < 45) {
        if (GLLastTextureBind == _tex) { GLLastTextureBind = 0; }
      }
      GX_GLCALL(glDeleteTextures, 1, &_tex);
    }
  }
};


template<int VER, GLenum TARGET>
class gx::GLTexture3DT
{
 public:
  using type = GLTexture3DT<VER,TARGET>;

  GLTexture3DT() = default;
  ~GLTexture3DT() { if (GLInitialized) cleanup(); }

  // prevent copy/assignment
  GLTexture3DT(const type&) = delete;
  type& operator=(const type&) = delete;

  // allow move/move-assign
  inline GLTexture3DT(type&& t) noexcept;
  inline type& operator=(type&& t) noexcept;

  // operators
  [[nodiscard]] explicit operator bool() const { return _tex; }

  // accessors
  [[nodiscard]] static GLenum target() { return TARGET; }
  [[nodiscard]] GLuint id() const { return _tex; }
  [[nodiscard]] GLenum internalFormat() const { return _internalformat; }
  [[nodiscard]] GLsizei levels() const { return _levels; }
  [[nodiscard]] GLsizei width() const { return _width; }
  [[nodiscard]] GLsizei height() const { return _height; }
  [[nodiscard]] GLsizei depth() const { return _depth; }

  // methods
  inline GLuint init(GLsizei levels, GLenum internalformat,
		     GLsizei width, GLsizei height, GLsizei depth);
    // for GL_TEXTURE_3D, GL_TEXTURE_2D_ARRAY, GL_TEXTURE_CUBE_MAP_ARRAY

  GLuint release() noexcept { return std::exchange(_tex, 0); }
    // releases ownership of managed texture object, returns object id

  inline static void bindUnit(GLuint unit, GLuint tex);
  void bindUnit(GLuint unit) const { bindUnit(unit, _tex); }
  static void unbindUnit(GLuint unit) { bindUnit(unit, 0); }

  inline void setSubImage3D(
    GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
    GLsizei width, GLsizei height, GLsizei depth,
    GLenum format, GLenum type, const void* pixels);

  template<typename PixelT>
  void setSubImage3D(GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
                     GLsizei width, GLsizei height, GLsizei depth,
                     GLenum format, const PixelT* pixels) {
    setSubImage3D(level, xoffset, yoffset, zoffset, width, height, depth,
                  format, GLType_v<PixelT>, pixels); }

  inline void getImage(
    GLint level, GLenum format, GLenum type, GLsizei bufSize, void* pixels);

  inline void generateMipmap();
    // not valid for GL_TEXTURE_2D_MULTISAMPLE_ARRAY

  inline void clear(GLint level);

  // set texture parameters
  inline void setParameter(GLenum pname, GLfloat param);
  inline void setParameter(GLenum pname, GLint param);
  inline void setParameterv(GLenum pname, const GLfloat* params);
  inline void setParameterv(GLenum pname, const GLint* params);
  inline void setParameterIv(GLenum pname, const GLint* params);
  inline void setParameterIv(GLenum pname, const GLuint* params);

  [[nodiscard]] static GLint maxSize() {
    GLint s = 0;
    GX_GLCALL(glGetIntegerv, GLTextureVals<TARGET>::pnameMaxSize, &s);
    return s;
  }

 private:
  GLuint _tex = 0;
  GLenum _internalformat = GL_NONE;
  GLsizei _levels = 0;
  GLsizei _width = 0;
  GLsizei _height = 0;
  GLsizei _depth = 0;

  void bindCheck() {
    if (GLLastTextureBind != _tex) {
      // don't care which unit is active
      GX_GLCALL(glBindTexture, TARGET, _tex);
      GLLastTextureBind = _tex;
    }
  }

  void cleanup() noexcept {
    if (_tex) {
      if constexpr (VER < 45) {
        if (GLLastTextureBind == _tex) { GLLastTextureBind = 0; }
      }
      GX_GLCALL(glDeleteTextures, 1, &_tex);
    }
  }
};


template<int VER>
class gx::GLTextureBuffer
{
 public:
  using type = GLTextureBuffer<VER>;

  GLTextureBuffer() = default;
  ~GLTextureBuffer() { if (GLInitialized) cleanup(); }

  // prevent copy/assignment
  GLTextureBuffer(const type&) = delete;
  type& operator=(const type&) = delete;

  // allow move/move-assign
  inline GLTextureBuffer(type&& t) noexcept;
  inline type& operator=(type&& t) noexcept;

  // operators
  [[nodiscard]] explicit operator bool() const { return _tex; }

  // accessors
  [[nodiscard]] static GLenum target() { return GL_TEXTURE_BUFFER; }
  [[nodiscard]] GLuint id() const { return _tex; }
  [[nodiscard]] GLenum internalFormat() const { return _internalformat; }

  // methods
  inline GLuint attachBuffer(GLenum internalformat, GLuint buffer);
  inline void detachBuffer();

  GLuint release() noexcept { return std::exchange(_tex, 0); }
    // releases ownership of managed texture object, returns object id

  inline static void bindUnit(GLuint unit, GLuint tex);
  void bindUnit(GLuint unit) const { bindUnit(unit, _tex); }
  static void unbindUnit(GLuint unit) { bindUnit(unit, 0); }

  [[nodiscard]] static GLint maxSize() {
    GLint s = 0;
    GX_GLCALL(glGetIntegerv, GLTextureVals<target()>::pnameMaxSize, &s);
    return s;
  }

 private:
  GLuint _tex = 0;
  GLenum _internalformat = GL_NONE;
  // track attached buffer ID?

  void bindCheck() {
    if (GLLastTextureBind != _tex) {
      // don't care which unit is active
      GX_GLCALL(glBindTexture, target(), _tex);
      GLLastTextureBind = _tex;
    }
  }

  void cleanup() noexcept {
    if (_tex) {
      if constexpr (VER < 45) {
        if (GLLastTextureBind == _tex) { GLLastTextureBind = 0; }
      }
      GX_GLCALL(glDeleteTextures, 1, &_tex);
    }
  }
};


// **** GLTexture1D implementation ****
template<int VER>
gx::GLTexture1D<VER>::GLTexture1D(GLTexture1D<VER>&& t) noexcept
  : _tex{t.release()}
{
  _internalformat = t._internalformat;
  _levels = t._levels;
  _width = t._width;
}

template<int VER>
gx::GLTexture1D<VER>&
gx::GLTexture1D<VER>::operator=(GLTexture1D<VER>&& t) noexcept
{
  if (this != &t) {
    cleanup();
    _tex = t.release();
    _internalformat = t._internalformat;
    _levels = t._levels;
    _width = t._width;
  }
  return *this;
}

template<int VER>
GLuint gx::GLTexture1D<VER>::init(
  GLsizei levels, GLenum internalformat, GLsizei width)
{
  cleanup();
  _internalformat = internalformat;
  _levels = levels;
  _width = width;
  if constexpr (VER < 42) {
    GX_GLCALL(glGenTextures, 1, &_tex);
    bindCheck();
    for (int i = 0; i < levels; ++i) {
      GX_GLCALL(glTexImage1D, target(), i, internalformat, width, 0,
                GL_RED, GL_UNSIGNED_BYTE, nullptr);
      width = std::max(1, (width / 2));
    }
  } else if constexpr (VER < 45) {
    GX_GLCALL(glGenTextures, 1, &_tex);
    bindCheck();
    GX_GLCALL(glTexStorage1D, target(), levels, internalformat, width);
  } else {
    GX_GLCALL(glCreateTextures, target(), 1, &_tex);
    GX_GLCALL(glTextureStorage1D, _tex, levels, internalformat, width);
  }
  return _tex;
}

template<int VER>
void gx::GLTexture1D<VER>::bindUnit(GLuint unit, GLuint tex)
{
  if constexpr (VER < 45) {
    GX_GLCALL(glActiveTexture, GL_TEXTURE0 + unit);
    GX_GLCALL(glBindTexture, target(), tex);
    GLLastTextureBind = tex;
  } else {
    GX_GLCALL(glBindTextureUnit, unit, tex);
  }
}

template<int VER>
void gx::GLTexture1D<VER>::setSubImage1D(
  GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void* pixels)
{
  GLSetUnpackAlignment(width, format, type);
  if constexpr (VER < 45) {
    bindCheck();
    GX_GLCALL(glTexSubImage1D, target(), level, xoffset, width, format, type, pixels);
  } else {
    GX_GLCALL(glTextureSubImage1D, _tex, level, xoffset, width, format, type, pixels);
  }
}

template<int VER>
void gx::GLTexture1D<VER>::getImage(
  GLint level, GLenum format, GLenum type, GLsizei bufSize, void* pixels)
{
  if constexpr (VER < 45) {
    bindCheck();
    GX_GLCALL(glGetTexImage, target(), level, format, type, pixels);
  } else {
    GX_GLCALL(glGetTextureImage, _tex, level, format, type, bufSize, pixels);
  }
}

template<int VER>
void gx::GLTexture1D<VER>::generateMipmap()
{
  if constexpr (VER < 45) {
    bindCheck();
    GX_GLCALL(glGenerateMipmap, target());
  } else {
    GX_GLCALL(glGenerateTextureMipmap, _tex);
  }
}

template<int VER>
void gx::GLTexture1D<VER>::clear(GLint level)
{
  const GLenum format = GLBaseFormat(_internalformat);
  const GLenum type = (format == GL_DEPTH_STENCIL)
    ? GL_UNSIGNED_INT_24_8 : GL_UNSIGNED_BYTE;

  if constexpr (VER < 44) {
    const int pixel_size = GLPixelSize(format, type);
    const auto empty = std::make_unique<GLubyte[]>(_width * pixel_size);
    setSubImage1D(level, 0, _width, format, empty.get());
  } else {
    GX_GLCALL(glClearTexImage, _tex, level, format, type, nullptr);
  }
}

template<int VER>
void gx::GLTexture1D<VER>::setParameter(GLenum pname, GLfloat param)
{
  if constexpr (VER < 45) {
    bindCheck();
    GX_GLCALL(glTexParameterf, target(), pname, param);
  } else {
    GX_GLCALL(glTextureParameterf, _tex, pname, param);
  }
}

template<int VER>
void gx::GLTexture1D<VER>::setParameter(GLenum pname, GLint param)
{
  if constexpr (VER < 45) {
    bindCheck();
    GX_GLCALL(glTexParameteri, target(), pname, param);
  } else {
    GX_GLCALL(glTextureParameteri, _tex, pname, param);
  }
}

template<int VER>
void gx::GLTexture1D<VER>::setParameterv(GLenum pname, const GLfloat* params)
{
  if constexpr (VER < 45) {
    bindCheck();
    GX_GLCALL(glTexParameterfv, target(), pname, params);
  } else {
    GX_GLCALL(glTextureParameterfv, _tex, pname, params);
  }
}

template<int VER>
void gx::GLTexture1D<VER>::setParameterv(GLenum pname, const GLint* params)
{
  if constexpr (VER < 45) {
    bindCheck();
    GX_GLCALL(glTexParameteriv, target(), pname, params);
  } else {
    GX_GLCALL(glTextureParameteriv, _tex, pname, params);
  }
}

template<int VER>
void gx::GLTexture1D<VER>::setParameterIv(GLenum pname, const GLint* params)
{
  if constexpr (VER < 45) {
    bindCheck();
    GX_GLCALL(glTexParameterIiv, target(), pname, params);
  } else {
    GX_GLCALL(glTextureParameterIiv, _tex, pname, params);
  }
}

template<int VER>
void gx::GLTexture1D<VER>::setParameterIv(GLenum pname, const GLuint* params)
{
  if constexpr (VER < 45) {
    bindCheck();
    GX_GLCALL(glTexParameterIuiv, target(), pname, params);
  } else {
    GX_GLCALL(glTextureParameterIuiv, _tex, pname, params);
  }
}


// **** GLTexture2DT implementation ****
template<int VER, GLenum TARGET>
gx::GLTexture2DT<VER,TARGET>::GLTexture2DT(GLTexture2DT<VER,TARGET>&& t) noexcept
  : _tex{t.release()}
{
  _internalformat = t._internalformat;
  _levels = t._levels;
  _width = t._width;
  _height = t._height;
}

template<int VER, GLenum TARGET>
gx::GLTexture2DT<VER,TARGET>&
gx::GLTexture2DT<VER,TARGET>::operator=(GLTexture2DT<VER,TARGET>&& t) noexcept
{
  if (this != &t) {
    cleanup();
    _tex = t.release();
    _internalformat = t._internalformat;
    _levels = t._levels;
    _width = t._width;
    _height = t._height;
  }
  return *this;
}

template<int VER, GLenum TARGET>
GLuint gx::GLTexture2DT<VER,TARGET>::init(
  GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)
{
  cleanup();
  _internalformat = internalformat;
  _levels = levels;
  _width = width;
  _height = height;
  if constexpr (VER < 42) {
    GX_GLCALL(glGenTextures, 1, &_tex);
    bindCheck();
    if constexpr (TARGET == GL_TEXTURE_CUBE_MAP) {
      // FIXME: move to dedicated GLTextureCubeMap class
      for (int i = 0; i < levels; ++i) {
        for (unsigned int f = 0; f < 6; ++f) {
          GX_GLCALL(glTexImage2D, GL_TEXTURE_CUBE_MAP_POSITIVE_X + f, i,
                    GLint(internalformat), width, height, 0, GL_RED,
                    GL_UNSIGNED_BYTE, nullptr);
        }
        width = std::max(1, (width / 2));
        height = std::max(1, (height / 2));
      }
    } else if constexpr (TARGET == GL_TEXTURE_1D_ARRAY
                         || TARGET == GL_PROXY_TEXTURE_1D_ARRAY) {
      for (int i = 0; i < levels; ++i) {
        GX_GLCALL(glTexImage2D, TARGET, i, internalformat, width, height, 0,
                  GL_RED, GL_UNSIGNED_BYTE, nullptr);
        width = std::max(1, (width / 2));
      }
    } else {
      for (int i = 0; i < levels; ++i) {
        GX_GLCALL(glTexImage2D, TARGET, i, GLint(internalformat),
                  width, height, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        width = std::max(1, (width / 2));
        height = std::max(1, (height / 2));
      }
    }
  } else if constexpr (VER < 45) {
    // FIXME: GL_TEXTURE_CUBE_MAP not handled
    GX_GLCALL(glGenTextures, 1, &_tex);
    bindCheck();
    GX_GLCALL(glTexStorage2D, TARGET, levels, internalformat, width, height);
  } else {
    // FIXME: GL_TEXTURE_CUBE_MAP not handled
    GX_GLCALL(glCreateTextures, TARGET, 1, &_tex);
    GX_GLCALL(glTextureStorage2D, _tex, levels, internalformat, width, height);
  }
  return _tex;
}

// TODO: init for GL_TEXTURE_2D_MULTISAMPLE
//   3.3 - glTexImage2DMultisample
//   4.5 - glTextureStorage2DMultisample

template<int VER, GLenum TARGET>
void gx::GLTexture2DT<VER,TARGET>::bindUnit(GLuint unit, GLuint tex)
{
  if constexpr (VER < 45) {
    GX_GLCALL(glActiveTexture, GL_TEXTURE0 + unit);
    GX_GLCALL(glBindTexture, TARGET, tex);
    GLLastTextureBind = tex;
  } else {
    GX_GLCALL(glBindTextureUnit, unit, tex);
  }
}

template<int VER, GLenum TARGET>
void gx::GLTexture2DT<VER,TARGET>::setSubImage2D(
  GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
  GLenum format, GLenum type, const void* pixels)
{
  GLSetUnpackAlignment(width, format, type);
  if constexpr (VER < 45) {
    bindCheck();
    GX_GLCALL(glTexSubImage2D, TARGET, level, xoffset, yoffset,
              width, height, format, type, pixels);
  } else {
    GX_GLCALL(glTextureSubImage2D, _tex, level, xoffset, yoffset,
              width, height, format, type, pixels);
  }
}

template<int VER, GLenum TARGET>
void gx::GLTexture2DT<VER,TARGET>::getImage(
  GLint level, GLenum format, GLenum type, GLsizei bufSize, void* pixels)
{
  if constexpr (VER < 45) {
    bindCheck();
    GX_GLCALL(glGetTexImage, TARGET, level, format, type, pixels);
  } else {
    GX_GLCALL(glGetTextureImage, _tex, level, format, type, bufSize, pixels);
  }
}

template<int VER, GLenum TARGET>
void gx::GLTexture2DT<VER,TARGET>::generateMipmap()
{
  if constexpr (VER < 45) {
    bindCheck();
    GX_GLCALL(glGenerateMipmap, TARGET);
  } else {
    GX_GLCALL(glGenerateTextureMipmap, _tex);
  }
}

template<int VER, GLenum TARGET>
void gx::GLTexture2DT<VER,TARGET>::clear(GLint level)
{
  const GLenum format = GLBaseFormat(_internalformat);
  const GLenum type = (format == GL_DEPTH_STENCIL)
    ? GL_UNSIGNED_INT_24_8 : GL_UNSIGNED_BYTE;

  if constexpr (VER < 44) {
    const int pixel_size = GLPixelSize(format, type);
    const auto empty = std::make_unique<GLubyte[]>(
      _width * _height * pixel_size);
    setSubImage2D(level, 0, 0, _width, _height, format, empty.get());
  } else {
    GX_GLCALL(glClearTexImage, _tex, level, format, type, nullptr);
  }
}

template<int VER, GLenum TARGET>
void gx::GLTexture2DT<VER,TARGET>::setParameter(GLenum pname, GLfloat param)
{
  if constexpr (VER < 45) {
    bindCheck();
    GX_GLCALL(glTexParameterf, TARGET, pname, param);
  } else {
    GX_GLCALL(glTextureParameterf, _tex, pname, param);
  }
}

template<int VER, GLenum TARGET>
void gx::GLTexture2DT<VER,TARGET>::setParameter(GLenum pname, GLint param)
{
  if constexpr (VER < 45) {
    bindCheck();
    GX_GLCALL(glTexParameteri, TARGET, pname, param);
  } else {
    GX_GLCALL(glTextureParameteri, _tex, pname, param);
  }
}

template<int VER, GLenum TARGET>
void gx::GLTexture2DT<VER,TARGET>::setParameterv(GLenum pname, const GLfloat* params)
{
  if constexpr (VER < 45) {
    bindCheck();
    GX_GLCALL(glTexParameterfv, TARGET, pname, params);
  } else {
    GX_GLCALL(glTextureParameterfv, _tex, pname, params);
  }
}

template<int VER, GLenum TARGET>
void gx::GLTexture2DT<VER,TARGET>::setParameterv(GLenum pname, const GLint* params)
{
  if constexpr (VER < 45) {
    bindCheck();
    GX_GLCALL(glTexParameteriv, TARGET, pname, params);
  } else {
    GX_GLCALL(glTextureParameteriv, _tex, pname, params);
  }
}

template<int VER, GLenum TARGET>
void gx::GLTexture2DT<VER,TARGET>::setParameterIv(GLenum pname, const GLint* params)
{
  if constexpr (VER < 45) {
    bindCheck();
    GX_GLCALL(glTexParameterIiv, TARGET, pname, params);
  } else {
    GX_GLCALL(glTextureParameterIiv, _tex, pname, params);
  }
}

template<int VER, GLenum TARGET>
void gx::GLTexture2DT<VER,TARGET>::setParameterIv(GLenum pname, const GLuint* params)
{
  if constexpr (VER < 45) {
    bindCheck();
    GX_GLCALL(glTexParameterIuiv, TARGET, pname, params);
  } else {
    GX_GLCALL(glTextureParameterIuiv, _tex, pname, params);
  }
}


// **** GLTexture3DT implementation ****
template<int VER, GLenum TARGET>
gx::GLTexture3DT<VER,TARGET>::GLTexture3DT(GLTexture3DT<VER,TARGET>&& t) noexcept
  : _tex{t.release()}
{
  _internalformat = t._internalformat;
  _levels = t._levels;
  _width = t._width;
  _height = t._height;
  _depth = t._depth;
}

template<int VER, GLenum TARGET>
gx::GLTexture3DT<VER,TARGET>&
gx::GLTexture3DT<VER,TARGET>::operator=(GLTexture3DT<VER,TARGET>&& t) noexcept
{
  if (this != &t) {
    cleanup();
    _tex = t.release();
    _internalformat = t._internalformat;
    _levels = t._levels;
    _width = t._width;
    _height = t._height;
    _depth = t._depth;
  }
  return *this;
}

template<int VER, GLenum TARGET>
GLuint gx::GLTexture3DT<VER,TARGET>::init(
  GLsizei levels, GLenum internalformat,
  GLsizei width, GLsizei height, GLsizei depth)
{
  cleanup();
  _internalformat = internalformat;
  _levels = levels;
  _width = width;
  _height = height;
  _depth = depth;
  if constexpr (VER < 42) {
    GX_GLCALL(glGenTextures, 1, &_tex);
    bindCheck();
    if constexpr (TARGET == GL_TEXTURE_3D || TARGET == GL_PROXY_TEXTURE_3D) {
      for (int i = 0; i < levels; ++i) {
        GX_GLCALL(glTexImage3D, TARGET, i, internalformat, width, height, depth,
                  0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        width = std::max(1, (width / 2));
        height = std::max(1, (height / 2));
        depth = std::max(1, (depth / 2));
      }
    } else {
      for (int i = 0; i < levels; ++i) {
        GX_GLCALL(glTexImage3D, TARGET, i, internalformat, width, height, depth,
                  0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        width = std::max(1, (width / 2));
        height = std::max(1, (height / 2));
      }
    }
  } else if constexpr (VER < 45) {
    GX_GLCALL(glGenTextures, 1, &_tex);
    bindCheck();
    GX_GLCALL(glTexStorage3D, TARGET, levels, internalformat,
              width, height, depth);
  } else {
    GX_GLCALL(glCreateTextures, TARGET, 1, &_tex);
    GX_GLCALL(glTextureStorage3D, _tex, levels, internalformat,
              width, height, depth);
  }
  return _tex;
}

// TODO: init for GL_TEXTURE_2D_MULTISAMPLE_ARRAY
//   3.3 - glTexImage3DMultisample
//   4.5 - glTextureStorage3DMultisample

template<int VER, GLenum TARGET>
void gx::GLTexture3DT<VER,TARGET>::bindUnit(GLuint unit, GLuint tex)
{
  if constexpr (VER < 45) {
    GX_GLCALL(glActiveTexture, GL_TEXTURE0 + unit);
    GX_GLCALL(glBindTexture, TARGET, tex);
    GLLastTextureBind = tex;
  } else {
    GX_GLCALL(glBindTextureUnit, unit, tex);
  }
}

template<int VER, GLenum TARGET>
void gx::GLTexture3DT<VER,TARGET>::setSubImage3D(
  GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
  GLsizei width, GLsizei height, GLsizei depth,
  GLenum format, GLenum type, const void* pixels)
{
  GLSetUnpackAlignment(width, format, type);
  if constexpr (VER < 45) {
    bindCheck();
    if constexpr (TARGET == GL_TEXTURE_CUBE_MAP) {
      // FIXME: move to dedicated GLTextureCubeMap class
      GX_GLCALL(glTexSubImage2D, GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + zoffset),
                level, xoffset, yoffset, width, height, format, type, pixels);
    } else {
      GX_GLCALL(glTexSubImage3D, TARGET, level, xoffset, yoffset, zoffset,
                width, height, depth, format, type, pixels);
    }
  } else {
    GX_GLCALL(glTextureSubImage3D, _tex, level, xoffset, yoffset, zoffset,
              width, height, depth, format, type, pixels);
  }
}

template<int VER, GLenum TARGET>
void gx::GLTexture3DT<VER,TARGET>::getImage(
  GLint level, GLenum format, GLenum type, GLsizei bufSize, void* pixels)
{
  if constexpr (VER < 45) {
    bindCheck();
    GX_GLCALL(glGetTexImage, TARGET, level, format, type, pixels);
  } else {
    GX_GLCALL(glGetTextureImage, _tex, level, format, type, bufSize, pixels);
  }
}

template<int VER, GLenum TARGET>
void gx::GLTexture3DT<VER,TARGET>::generateMipmap()
{
  if constexpr (VER < 45) {
    bindCheck();
    GX_GLCALL(glGenerateMipmap, TARGET);
  } else {
    GX_GLCALL(glGenerateTextureMipmap, _tex);
  }
}

template<int VER, GLenum TARGET>
void gx::GLTexture3DT<VER,TARGET>::clear(GLint level)
{
  const GLenum format = GLBaseFormat(_internalformat);
  const GLenum type = (format == GL_DEPTH_STENCIL)
    ? GL_UNSIGNED_INT_24_8 : GL_UNSIGNED_BYTE;

  if constexpr (VER < 44) {
    const int pixel_size = GLPixelSize(format, type);
    const auto empty = std::make_unique<GLubyte[]>(
      _width * _height * _depth * pixel_size);
    setSubImage3D(level, 0, 0, 0, _width, _height, _depth, format, empty.get());
  } else {
    GX_GLCALL(glClearTexImage, _tex, level, format, type, nullptr);
  }
}

template<int VER, GLenum TARGET>
void gx::GLTexture3DT<VER,TARGET>::setParameter(GLenum pname, GLfloat param)
{
  if constexpr (VER < 45) {
    bindCheck();
    GX_GLCALL(glTexParameterf, TARGET, pname, param);
  } else {
    GX_GLCALL(glTextureParameterf, _tex, pname, param);
  }
}

template<int VER, GLenum TARGET>
void gx::GLTexture3DT<VER,TARGET>::setParameter(GLenum pname, GLint param)
{
  if constexpr (VER < 45) {
    bindCheck();
    GX_GLCALL(glTexParameteri, TARGET, pname, param);
  } else {
    GX_GLCALL(glTextureParameteri, _tex, pname, param);
  }
}

template<int VER, GLenum TARGET>
void gx::GLTexture3DT<VER,TARGET>::setParameterv(GLenum pname, const GLfloat* params)
{
  if constexpr (VER < 45) {
    bindCheck();
    GX_GLCALL(glTexParameterfv, TARGET, pname, params);
  } else {
    GX_GLCALL(glTextureParameterfv, _tex, pname, params);
  }
}

template<int VER, GLenum TARGET>
void gx::GLTexture3DT<VER,TARGET>::setParameterv(GLenum pname, const GLint* params)
{
  if constexpr (VER < 45) {
    bindCheck();
    GX_GLCALL(glTexParameteriv, TARGET, pname, params);
  } else {
    GX_GLCALL(glTextureParameteriv, _tex, pname, params);
  }
}

template<int VER, GLenum TARGET>
void gx::GLTexture3DT<VER,TARGET>::setParameterIv(GLenum pname, const GLint* params)
{
  if constexpr (VER < 45) {
    bindCheck();
    GX_GLCALL(glTexParameterIiv, TARGET, pname, params);
  } else {
    GX_GLCALL(glTextureParameterIiv, _tex, pname, params);
  }
}

template<int VER, GLenum TARGET>
void gx::GLTexture3DT<VER,TARGET>::setParameterIv(GLenum pname, const GLuint* params)
{
  if constexpr (VER < 45) {
    bindCheck();
    GX_GLCALL(glTexParameterIuiv, TARGET, pname, params);
  } else {
    GX_GLCALL(glTextureParameterIuiv, _tex, pname, params);
  }
}


// **** GLTextureBuffer implementation ****
template<int VER>
gx::GLTextureBuffer<VER>::GLTextureBuffer(GLTextureBuffer<VER>&& t) noexcept
  : _tex{t.release()}
{
  _internalformat = t._internalformat;
}

template<int VER>
gx::GLTextureBuffer<VER>&
gx::GLTextureBuffer<VER>::operator=(GLTextureBuffer<VER>&& t) noexcept
{
  if (this != &t) {
    cleanup();
    _tex = t.release();
    _internalformat = t._internalformat;
  }
  return *this;
}

template<int VER>
GLuint gx::GLTextureBuffer<VER>::attachBuffer(
  GLenum internalformat, GLuint buffer)
{
  _internalformat = internalformat;
  if constexpr (VER < 45) {
    if (!_tex) { GX_GLCALL(glGenTextures, 1, &_tex); }
    bindCheck();
    GX_GLCALL(glTexBuffer, target(), internalformat, buffer);
  } else {
    if (!_tex) { GX_GLCALL(glCreateTextures, target(), 1, &_tex); }
    GX_GLCALL(glTextureBuffer, _tex, internalformat, buffer);
  }
  return _tex;
}

template<int VER>
void gx::GLTextureBuffer<VER>::detachBuffer()
{
  if constexpr (VER < 45) {
    bindCheck();
    GX_GLCALL(glTexBuffer, target(), _internalformat, 0);
  } else {
    GX_GLCALL(glTextureBuffer, _tex, _internalformat, 0);
  }
}

template<int VER>
void gx::GLTextureBuffer<VER>::bindUnit(GLuint unit, GLuint tex)
{
  if constexpr (VER < 45) {
    GX_GLCALL(glActiveTexture, GL_TEXTURE0 + unit);
    GX_GLCALL(glBindTexture, target(), tex);
    GLLastTextureBind = tex;
  } else {
    GX_GLCALL(glBindTextureUnit, unit, tex);
  }
}
