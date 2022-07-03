//
// gx/GLTexture.hh
// Copyright (C) 2022 Richard Bradley
//
// wrapper for OpenGL texture object
// (texture target as a template parameter)
//

#pragma once
#include "OpenGL.hh"
#include "Utility.hh"
#if defined(GX_GL33) || defined(GX_GL42) || defined(GX_GL43)
#include <memory>
#endif


inline namespace GX_GLNAMESPACE {

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


// **** Main texture template definition ****
template<GLenum TARGET>
class GLTextureT
{
 public:
  GLTextureT() = default;
  inline GLTextureT(GLTextureT<TARGET>&& t) noexcept;
  ~GLTextureT() { if (GLInitialized) cleanup(); }

  // operators
  inline GLTextureT<TARGET>& operator=(GLTextureT<TARGET>&& t) noexcept;
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
  inline GLuint init(GLsizei levels, GLenum internalformat, GLsizei width);
    // for GL_TEXTURE_1D
  inline GLuint init(GLsizei levels, GLenum internalformat,
		     GLsizei width, GLsizei height);
    // for GL_TEXTURE_2D, GL_TEXTURE_1D_ARRAY, GL_TEXTURE_RECTANGLE,
    //   GL_TEXTURE_CUBE_MAP
  inline GLuint init(GLsizei levels, GLenum internalformat,
		     GLsizei width, GLsizei height, GLsizei depth);
    // for GL_TEXTURE_3D, GL_TEXTURE_2D_ARRAY, GL_TEXTURE_CUBE_MAP_ARRAY

  inline GLuint attachBuffer(GLenum internalformat, GLuint buffer);
  inline void detachBuffer();
    // for GL_TEXTURE_BUFFER

  GLuint release() noexcept { return gx::exchange(_tex, 0); }
    // releases ownership of managed texture object, returns object id

  inline static void bindUnit(GLuint unit, GLuint tex);
  void bindUnit(GLuint unit) const { bindUnit(unit, _tex); }
  static void unbindUnit(GLuint unit) { bindUnit(unit, 0); }

  inline void setSubImage1D(
    GLint level, GLint xoffset, GLsizei width,
    GLenum format, GLenum type, const void* pixels);
  inline void setSubImage2D(
    GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
    GLenum format, GLenum type, const void* pixels);
  inline void setSubImage3D(
    GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
    GLsizei width, GLsizei height, GLsizei depth,
    GLenum format, GLenum type, const void* pixels);

  template<typename PixelT>
  void setSubImage1D(GLint level, GLint xoffset, GLsizei width,
                     GLenum format, const PixelT* pixels) {
    setSubImage1D(level, xoffset, width, format, GLType_v<PixelT>, pixels); }
  template<typename PixelT>
  void setSubImage2D(GLint level, GLint xoffset, GLint yoffset, GLsizei width,
                     GLsizei height, GLenum format, const PixelT* pixels) {
    setSubImage2D(level, xoffset, yoffset, width, height, format,
                  GLType_v<PixelT>, pixels); }
  template<typename PixelT>
  void setSubImage3D(GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
                     GLsizei width, GLsizei height, GLsizei depth,
                     GLenum format, const PixelT* pixels) {
    setSubImage3D(level, xoffset, yoffset, zoffset, width, height, depth,
                  format, GLType_v<PixelT>, pixels); }

  inline void getImage(
    GLint level, GLenum format, GLenum type, GLsizei bufSize, void* pixels);
    // not valid for GL_TEXTURE_BUFFER

  inline void generateMipmap();
    // for GL_TEXTURE_1D, GL_TEXTURE_2D, GL_TEXTURE_3D, GL_TEXTURE_1D_ARRAY,
    //   GL_TEXTURE_2D_ARRAY, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_ARRAY
    // not valid for GL_TEXTURE_RECTANGLE, GL_TEXTURE_BUFFER,
    //   GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_2D_MULTISAMPLE_ARRAY

  inline void clear(GLint level);
    // not valid for GL_TEXTURE_BUFFER

  [[nodiscard]] inline float coordX(float x) const;
  [[nodiscard]] inline float coordY(float y) const;
    // return texture coordinate for x/y pixel coord

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

#if defined(GX_GL33) || defined(GX_GL42) || defined(GX_GL43)
  void bindCheck() {
    if (GLLastTextureBind != _tex) {
      // don't care which unit is active
      GX_GLCALL(glBindTexture, TARGET, _tex);
      GLLastTextureBind = _tex;
    }
  }
#endif
  static void setUnpackAlignment(GLsizei width, GLenum format, GLenum type);
  inline void cleanup() noexcept;

  // prevent copy/assignment
  GLTextureT(const GLTextureT<TARGET>&) = delete;
  GLTextureT<TARGET>& operator=(const GLTextureT<TARGET>&) = delete;
};


// **** Helper Type Aliases ****
using GLTexture1D = GLTextureT<GL_TEXTURE_1D>;
using GLTexture2D = GLTextureT<GL_TEXTURE_2D>;
using GLTexture3D = GLTextureT<GL_TEXTURE_3D>;
using GLTexture1DArray = GLTextureT<GL_TEXTURE_1D_ARRAY>;
using GLTexture2DArray = GLTextureT<GL_TEXTURE_2D_ARRAY>;
using GLTextureRectangle = GLTextureT<GL_TEXTURE_RECTANGLE>;
using GLTextureCubeMap = GLTextureT<GL_TEXTURE_CUBE_MAP>;
using GLTextureCubeMapArray = GLTextureT<GL_TEXTURE_CUBE_MAP_ARRAY>;
using GLTextureBuffer = GLTextureT<GL_TEXTURE_BUFFER>;
using GLTexture2DMultisample = GLTextureT<GL_TEXTURE_2D_MULTISAMPLE>;
using GLTexture2DMultisampleArray = GLTextureT<GL_TEXTURE_2D_MULTISAMPLE_ARRAY>;


// **** Inline Implementations ****
template<GLenum TARGET>
GLTextureT<TARGET>::GLTextureT(GLTextureT<TARGET>&& t) noexcept
  : _tex(t.release())
{
  _internalformat = t._internalformat;
  _levels = t._levels;
  _width = t._width;
  _height = t._height;
  _depth = t._depth;
}

template<GLenum TARGET>
GLTextureT<TARGET>&
GLTextureT<TARGET>::operator=(GLTextureT<TARGET>&& t) noexcept
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

template<GLenum TARGET>
GLuint GLTextureT<TARGET>::init(
  GLsizei levels, GLenum internalformat, GLsizei width)
{
  cleanup();
  _internalformat = internalformat;
  _levels = levels;
  _width = width;
  _height = 0;
  _depth = 0;
#ifdef GX_GL33
  GX_GLCALL(glGenTextures, 1, &_tex);
  bindCheck();
  for (int i = 0; i < levels; ++i) {
    GX_GLCALL(glTexImage1D, TARGET, i, internalformat, width, 0,
              GL_RED, GL_UNSIGNED_BYTE, nullptr);
    width = std::max(1, (width / 2));
  }
#elif defined(GX_GL42) || defined(GX_GL43)
  GX_GLCALL(glGenTextures, 1, &_tex);
  bindCheck();
  GX_GLCALL(glTexStorage1D, TARGET, levels, internalformat, width);
#else
  GX_GLCALL(glCreateTextures, TARGET, 1, &_tex);
  GX_GLCALL(glTextureStorage1D, _tex, levels, internalformat, width);
#endif
  return _tex;
}

template<GLenum TARGET>
GLuint GLTextureT<TARGET>::init(
  GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)
{
  cleanup();
  _internalformat = internalformat;
  _levels = levels;
  _width = width;
  _height = height;
  _depth = 0;
#ifdef GX_GL33
  GX_GLCALL(glGenTextures, 1, &_tex);
  bindCheck();
  if constexpr (TARGET == GL_TEXTURE_CUBE_MAP) {
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
#elif defined(GX_GL42) || defined(GX_GL43)
  GX_GLCALL(glGenTextures, 1, &_tex);
  bindCheck();
  GX_GLCALL(glTexStorage2D, TARGET, levels, internalformat, width, height);
#else
  GX_GLCALL(glCreateTextures, TARGET, 1, &_tex);
  GX_GLCALL(glTextureStorage2D, _tex, levels, internalformat, width, height);
#endif
  return _tex;
}

template<GLenum TARGET>
GLuint GLTextureT<TARGET>::init(
  GLsizei levels, GLenum internalformat,
  GLsizei width, GLsizei height, GLsizei depth)
{
  cleanup();
  _internalformat = internalformat;
  _levels = levels;
  _width = width;
  _height = height;
  _depth = depth;
#ifdef GX_GL33
  GX_GLCALL(glGenTextures, 1, &_tex);
  bindCheck();
  if constexpr (TARGET == GL_TEXTURE_3D || TARGET == GL_PROXY_TEXTURE_3D) {
    for (int i = 0; i < levels; ++i) {
      GX_GLCALL(glTexImage3D, TARGET, i, internalformat, width, height, depth, 0,
                GL_RED, GL_UNSIGNED_BYTE, nullptr);
      width = std::max(1, (width / 2));
      height = std::max(1, (height / 2));
      depth = std::max(1, (depth / 2));
    }
  } else {
    for (int i = 0; i < levels; ++i) {
      GX_GLCALL(glTexImage3D, TARGET, i, internalformat, width, height, depth, 0,
                GL_RED, GL_UNSIGNED_BYTE, nullptr);
      width = std::max(1, (width / 2));
      height = std::max(1, (height / 2));
    }
  }
#elif defined(GX_GL42) || defined(GX_GL43)
  GX_GLCALL(glGenTextures, 1, &_tex);
  bindCheck();
  GX_GLCALL(glTexStorage3D, TARGET, levels, internalformat, width, height, depth);
#else
  GX_GLCALL(glCreateTextures, TARGET, 1, &_tex);
  GX_GLCALL(glTextureStorage3D, _tex, levels, internalformat, width, height, depth);
#endif
  return _tex;
}

// TODO: init for GL_TEXTURE_2D_MULTISAMPLE
//   3.3 - glTexImage2DMultisample
//   4.5 - glTextureStorage2DMultisample

// TODO: init for GL_TEXTURE_2D_MULTISAMPLE_ARRAY
//   3.3 - glTexImage3DMultisample
//   4.5 - glTextureStorage3DMultisample

template<GLenum TARGET>
GLuint GLTextureT<TARGET>::attachBuffer(GLenum internalformat, GLuint buffer)
{
  _internalformat = internalformat;
  _levels = 0;
  _width = 0;
  _height = 0;
  _depth = 0;
#if defined(GX_GL33) || defined(GX_GL42) || defined(GX_GL43)
  if (!_tex) { GX_GLCALL(glGenTextures, 1, &_tex); }
  bindCheck();
  GX_GLCALL(glTexBuffer, TARGET, internalformat, buffer);
#else
  if (!_tex) { GX_GLCALL(glCreateTextures, TARGET, 1, &_tex); }
  GX_GLCALL(glTextureBuffer, _tex, internalformat, buffer);
#endif
  return _tex;
}

template<GLenum TARGET>
void GLTextureT<TARGET>::detachBuffer()
{
#if defined(GX_GL33) || defined(GX_GL42) || defined(GX_GL43)
  bindCheck();
  GX_GLCALL(glTexBuffer, TARGET, _internalformat, 0);
#else
  GX_GLCALL(glTextureBuffer, _tex, _internalformat, 0);
#endif
}

template<GLenum TARGET>
void GLTextureT<TARGET>::bindUnit(GLuint unit, GLuint tex)
{
#if defined(GX_GL33) || defined(GX_GL42) || defined(GX_GL43)
  GX_GLCALL(glActiveTexture, GL_TEXTURE0 + unit);
  GX_GLCALL(glBindTexture, TARGET, tex);
  GLLastTextureBind = tex;
#else
  GX_GLCALL(glBindTextureUnit, unit, tex);
#endif
}

template<GLenum TARGET>
void GLTextureT<TARGET>::setSubImage1D(
  GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void* pixels)
{
  setUnpackAlignment(width, format, type);
#if defined(GX_GL33) || defined(GX_GL42) || defined(GX_GL43)
  bindCheck();
  GX_GLCALL(glTexSubImage1D, TARGET, level, xoffset, width, format, type, pixels);
#else
  GX_GLCALL(glTextureSubImage1D, _tex, level, xoffset, width, format, type, pixels);
#endif
}

template<GLenum TARGET>
void GLTextureT<TARGET>::setSubImage2D(
  GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
  GLenum format, GLenum type, const void* pixels)
{
  setUnpackAlignment(width, format, type);
#if defined(GX_GL33) || defined(GX_GL42) || defined(GX_GL43)
  bindCheck();
  GX_GLCALL(glTexSubImage2D, TARGET, level, xoffset, yoffset,
            width, height, format, type, pixels);
#else
  GX_GLCALL(glTextureSubImage2D, _tex, level, xoffset, yoffset,
            width, height, format, type, pixels);
#endif
}

template<GLenum TARGET>
void GLTextureT<TARGET>::setSubImage3D(
  GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
  GLsizei width, GLsizei height, GLsizei depth,
  GLenum format, GLenum type, const void* pixels)
{
  setUnpackAlignment(width, format, type);
#if defined(GX_GL33) || defined(GX_GL42) || defined(GX_GL43)
  bindCheck();
  if constexpr (TARGET == GL_TEXTURE_CUBE_MAP) {
    GX_GLCALL(glTexSubImage2D, GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + zoffset),
              level, xoffset, yoffset, width, height, format, type, pixels);
  } else {
    GX_GLCALL(glTexSubImage3D, TARGET, level, xoffset, yoffset, zoffset,
              width, height, depth, format, type, pixels);
  }
#else
  GX_GLCALL(glTextureSubImage3D, _tex, level, xoffset, yoffset, zoffset,
            width, height, depth, format, type, pixels);
#endif
}

template<GLenum TARGET>
void GLTextureT<TARGET>::getImage(
  GLint level, GLenum format, GLenum type, GLsizei bufSize, void* pixels)
{
#if defined(GX_GL33) || defined(GX_GL42) || defined(GX_GL43)
  bindCheck();
  GX_GLCALL(glGetTexImage, TARGET, level, format, type, pixels);
#else
  GX_GLCALL(glGetTextureImage, _tex, level, format, type, bufSize, pixels);
#endif
}

template<GLenum TARGET>
void GLTextureT<TARGET>::generateMipmap()
{
#if defined(GX_GL33) || defined(GX_GL42) || defined(GX_GL43)
  bindCheck();
  GX_GLCALL(glGenerateMipmap, TARGET);
#else
  GX_GLCALL(glGenerateTextureMipmap, _tex);
#endif
}

template<GLenum TARGET>
void GLTextureT<TARGET>::clear(GLint level)
{
  const GLenum format = GLBaseFormat(_internalformat);
  const GLenum type = (format == GL_DEPTH_STENCIL)
    ? GL_UNSIGNED_INT_24_8 : GL_UNSIGNED_BYTE;

#if defined(GX_GL33) || defined(GX_GL42) || defined(GX_GL43)
  const int pixel_size = GLPixelSize(format, type);
  const auto empty = std::make_unique<GLubyte[]>(
    _width * std::max(_height,1) * std::max(_depth,1) * pixel_size);
  if (_depth > 0) {
    setSubImage3D(level, 0, 0, 0, _width, _height, _depth, format, empty.get());
  } else if (_height > 0) {
    setSubImage2D(level, 0, 0, _width, _height, format, empty.get());
  } else {
    setSubImage1D(level, 0, _width, format, empty.get());
  }
#else
  GX_GLCALL(glClearTexImage, _tex, level, format, type, nullptr); // GL4.4
#endif
}

template<GLenum TARGET>
float GLTextureT<TARGET>::coordX(float x) const
{
  if constexpr (TARGET == GL_TEXTURE_RECTANGLE) {
    return x;
  } else {
    return x / float(_width);
  }
}

template<GLenum TARGET>
float GLTextureT<TARGET>::coordY(float y) const
{
  if constexpr (TARGET == GL_TEXTURE_RECTANGLE) {
    return y;
  } else {
    return y / float(_height);
  }
}

template<GLenum TARGET>
void GLTextureT<TARGET>::setUnpackAlignment(
  GLsizei width, GLenum format, GLenum type)
{
  const int x = width * GLPixelSize(format, type);
  int align = 8;
  if (x & 1) { align = 1; }
  else if (x & 2) { align = 2; }
  else if (x & 4) { align = 4; }

  GX_GLCALL(glPixelStorei, GL_UNPACK_ALIGNMENT, align);
}

template<GLenum TARGET>
void GLTextureT<TARGET>::cleanup() noexcept
{
  if (_tex) {
#if defined(GX_GL33) || defined(GX_GL42) || defined(GX_GL43)
    if (GLLastTextureBind == _tex) { GLLastTextureBind = 0; }
#endif
    GX_GLCALL(glDeleteTextures, 1, &_tex);
  }
}

template<GLenum TARGET>
void GLTextureT<TARGET>::setParameter(GLenum pname, GLfloat param)
{
#if defined(GX_GL33) || defined(GX_GL42) || defined(GX_GL43)
  bindCheck();
  GX_GLCALL(glTexParameterf, TARGET, pname, param);
#else
  GX_GLCALL(glTextureParameterf, _tex, pname, param);
#endif
}

template<GLenum TARGET>
void GLTextureT<TARGET>::setParameter(GLenum pname, GLint param)
{
#if defined(GX_GL33) || defined(GX_GL42) || defined(GX_GL43)
  bindCheck();
  GX_GLCALL(glTexParameteri, TARGET, pname, param);
#else
  GX_GLCALL(glTextureParameteri, _tex, pname, param);
#endif
}

template<GLenum TARGET>
void GLTextureT<TARGET>::setParameterv(GLenum pname, const GLfloat* params)
{
#if defined(GX_GL33) || defined(GX_GL42) || defined(GX_GL43)
  bindCheck();
  GX_GLCALL(glTexParameterfv, TARGET, pname, params);
#else
  GX_GLCALL(glTextureParameterfv, _tex, pname, params);
#endif
}

template<GLenum TARGET>
void GLTextureT<TARGET>::setParameterv(GLenum pname, const GLint* params)
{
#if defined(GX_GL33) || defined(GX_GL42) || defined(GX_GL43)
  bindCheck();
  GX_GLCALL(glTexParameteriv, TARGET, pname, params);
#else
  GX_GLCALL(glTextureParameteriv, _tex, pname, params);
#endif
}

template<GLenum TARGET>
void GLTextureT<TARGET>::setParameterIv(GLenum pname, const GLint* params)
{
#if defined(GX_GL33) || defined(GX_GL42) || defined(GX_GL43)
  bindCheck();
  GX_GLCALL(glTexParameterIiv, TARGET, pname, params);
#else
  GX_GLCALL(glTextureParameterIiv, _tex, pname, params);
#endif
}

template<GLenum TARGET>
void GLTextureT<TARGET>::setParameterIv(GLenum pname, const GLuint* params)
{
#if defined(GX_GL33) || defined(GX_GL42) || defined(GX_GL43)
  bindCheck();
  GX_GLCALL(glTexParameterIuiv, TARGET, pname, params);
#else
  GX_GLCALL(glTextureParameterIuiv, _tex, pname, params);
#endif
}

} // end GX_GLNAMESPACE
