//
// gx/GLTexture.hh
// Copyright (C) 2023 Richard Bradley
//
// wrappers for OpenGL texture objects
//

// TODO: add GLTextureCubeMapArray
//   (requires GL4.0 or ARB_texture_cube_map_array)
//
// REFERENCES:
//   https://www.khronos.org/opengl/wiki/Cubemap_Texture
//   https://www.khronos.org/opengl/wiki/Array_Texture
//   https://www.khronos.org/opengl/wiki/Texture_Storage
//   https://registry.khronos.org/OpenGL/extensions/ARB/ARB_direct_state_access.txt
//   https://registry.khronos.org/OpenGL/extensions/ARB/ARB_texture_cube_map_array.txt

#pragma once
#include "OpenGL.hh"
#include <utility>
#include <memory>

namespace gx {
  template<int VER>                class GLTexture1D;
  template<int VER, GLenum TARGET> class GLTexture2DT;
  template<int VER, GLenum TARGET> class GLTexture3DT;
  template<int VER>                class GLTextureCubeMap;
  template<int VER>                class GLTextureBuffer;
  template<int VER, GLenum TARGET> struct GLTextureHandle;


  // **** Helper type aliases ****
  template<int VER>
  using GLTexture2D = GLTexture2DT<VER,GL_TEXTURE_2D>;

  template<int VER>
  using GLTexture1DArray = GLTexture2DT<VER,GL_TEXTURE_1D_ARRAY>;

  template<int VER>
  using GLTextureRectangle = GLTexture2DT<VER,GL_TEXTURE_RECTANGLE>;

  template<int VER>
  using GLTexture2DMultisample = GLTexture2DT<VER,GL_TEXTURE_2D_MULTISAMPLE>;

  template<int VER>
  using GLTexture3D = GLTexture3DT<VER,GL_TEXTURE_3D>;

  template<int VER>
  using GLTexture2DArray = GLTexture3DT<VER,GL_TEXTURE_2D_ARRAY>;

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

  // operators
  [[nodiscard]] explicit operator bool() const { return _tex.id != 0; }

  // accessors
  [[nodiscard]] static GLenum target() { return GL_TEXTURE_1D; }
  [[nodiscard]] GLuint id() const { return _tex.id; }
  [[nodiscard]] GLenum internalFormat() const { return _internalformat; }
  [[nodiscard]] GLsizei levels() const { return _levels; }
  [[nodiscard]] GLsizei width() const { return _width; }

  // methods
  inline GLuint init(GLsizei levels, GLenum internalformat, GLsizei width);

  GLuint release() noexcept { return _tex.release(); }
    // releases ownership of managed texture object, returns object id

  void bindUnit(GLuint unit) const { _tex.bindUnit(unit); }

  inline void setSubImage(
    GLint level, GLint xoffset, GLsizei width,
    GLenum format, GLenum type, const void* pixels);

  template<typename PixelT>
  void setSubImage(GLint level, GLint xoffset, GLsizei width,
                   GLenum format, const PixelT* pixels) {
    setSubImage(level, xoffset, width, format, GLType_v<PixelT>, pixels); }

  void getImage(
    GLint level, GLenum format, GLenum type, GLsizei bufSize, void* pixels) {
    _tex.getImage(level, format, type, bufSize, pixels); }

  void generateMipmap() { _tex.generateMipmap(); }
  inline void clear(GLint level);

  // texture parameters
  void setParameter(GLenum pname, GLfloat param) {
    _tex.setParameterf(pname, param); }
  void setParameter(GLenum pname, GLint param) {
    _tex.setParameteri(pname, param); }
  void setParameterv(GLenum pname, const GLfloat* params) {
    _tex.setParameterfv(pname, params); }
  void setParameterv(GLenum pname, const GLint* params) {
    _tex.setParameteriv(pname, params); }
  void setParameterIv(GLenum pname, const GLint* params) {
    _tex.setParameterIiv(pname, params); }
  void setParameterIv(GLenum pname, const GLuint* params) {
    _tex.setParameterIiuv(pname, params); }

  void getParameterv(GLenum pname, GLfloat* params) {
    _tex.getParameterfv(pname, params); }
  void getParameterv(GLenum pname, GLint* params) {
    _tex.getParameteriv(pname, params); }
  void getParameterIv(GLenum pname, GLint* params) {
    _tex.getParameterIiv(pname, params); }
  void getParameterIv(GLenum pname, GLuint* params) {
    _tex.getParameterIuiv(pname, params); }

  [[nodiscard]] static GLint maxSize() {
    GLint s = 0;
    GX_GLCALL(glGetIntegerv, GLTextureVals<target()>::pnameMaxSize, &s);
    return s;
  }

 private:
  GLTextureHandle<VER,target()> _tex;
  GLenum _internalformat = GL_NONE;
  GLsizei _levels = 0;
  GLsizei _width = 0;
};


template<int VER, GLenum TARGET>
class gx::GLTexture2DT
{
 public:
  using type = GLTexture2DT<VER,TARGET>;

  // operators
  [[nodiscard]] explicit operator bool() const { return _tex.id != 0; }

  // accessors
  [[nodiscard]] static GLenum target() { return TARGET; }
  [[nodiscard]] GLuint id() const { return _tex.id; }
  [[nodiscard]] GLenum internalFormat() const { return _internalformat; }
  [[nodiscard]] GLsizei levels() const { return _levels; }
  [[nodiscard]] GLsizei width() const { return _width; }
  [[nodiscard]] GLsizei height() const { return _height; }

  // methods
  inline GLuint init(GLsizei levels, GLenum internalformat,
		     GLsizei width, GLsizei height);
    // for GL_TEXTURE_2D, GL_TEXTURE_1D_ARRAY, GL_TEXTURE_RECTANGLE

  GLuint release() noexcept { return _tex.release(); }
    // releases ownership of managed texture object, returns object id

  void bindUnit(GLuint unit) const { _tex.bindUnit(unit); }

  inline void setSubImage(
    GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
    GLenum format, GLenum type, const void* pixels);

  template<typename PixelT>
  void setSubImage(GLint level, GLint xoffset, GLint yoffset, GLsizei width,
                   GLsizei height, GLenum format, const PixelT* pixels) {
    setSubImage(level, xoffset, yoffset, width, height, format,
                GLType_v<PixelT>, pixels); }

  void getImage(
    GLint level, GLenum format, GLenum type, GLsizei bufSize, void* pixels) {
    _tex.getImage(level, format, type, bufSize, pixels); }

  void generateMipmap() { _tex.generateMipmap(); }
    // not valid for GL_TEXTURE_RECTANGLE, GL_TEXTURE_2D_MULTISAMPLE

  inline void clear(GLint level);

  // texture parameters
  void setParameter(GLenum pname, GLfloat param) {
    _tex.setParameterf(pname, param); }
  void setParameter(GLenum pname, GLint param) {
    _tex.setParameteri(pname, param); }
  void setParameterv(GLenum pname, const GLfloat* params) {
    _tex.setParameterfv(pname, params); }
  void setParameterv(GLenum pname, const GLint* params) {
    _tex.setParameteriv(pname, params); }
  void setParameterIv(GLenum pname, const GLint* params) {
    _tex.setParameterIiv(pname, params); }
  void setParameterIv(GLenum pname, const GLuint* params) {
    _tex.setParameterIiuv(pname, params); }

  void getParameterv(GLenum pname, GLfloat* params) {
    _tex.getParameterfv(pname, params); }
  void getParameterv(GLenum pname, GLint* params) {
    _tex.getParameteriv(pname, params); }
  void getParameterIv(GLenum pname, GLint* params) {
    _tex.getParameterIiv(pname, params); }
  void getParameterIv(GLenum pname, GLuint* params) {
    _tex.getParameterIuiv(pname, params); }

  [[nodiscard]] static GLint maxSize() {
    GLint s = 0;
    GX_GLCALL(glGetIntegerv, GLTextureVals<TARGET>::pnameMaxSize, &s);
    return s;
  }

 private:
  GLTextureHandle<VER,TARGET> _tex;
  GLenum _internalformat = GL_NONE;
  GLsizei _levels = 0;
  GLsizei _width = 0;
  GLsizei _height = 0;
};


template<int VER, GLenum TARGET>
class gx::GLTexture3DT
{
 public:
  using type = GLTexture3DT<VER,TARGET>;

  // operators
  [[nodiscard]] explicit operator bool() const { return _tex.id != 0; }

  // accessors
  [[nodiscard]] static GLenum target() { return TARGET; }
  [[nodiscard]] GLuint id() const { return _tex.id; }
  [[nodiscard]] GLenum internalFormat() const { return _internalformat; }
  [[nodiscard]] GLsizei levels() const { return _levels; }
  [[nodiscard]] GLsizei width() const { return _width; }
  [[nodiscard]] GLsizei height() const { return _height; }
  [[nodiscard]] GLsizei depth() const { return _depth; }

  // methods
  inline GLuint init(GLsizei levels, GLenum internalformat,
		     GLsizei width, GLsizei height, GLsizei depth);
    // for GL_TEXTURE_3D, GL_TEXTURE_2D_ARRAY

  GLuint release() noexcept { return _tex.release(); }
    // releases ownership of managed texture object, returns object id

  void bindUnit(GLuint unit) const { _tex.bindUnit(unit); }

  inline void setSubImage(
    GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
    GLsizei width, GLsizei height, GLsizei depth,
    GLenum format, GLenum type, const void* pixels);

  template<typename PixelT>
  void setSubImage(GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
                   GLsizei width, GLsizei height, GLsizei depth,
                   GLenum format, const PixelT* pixels) {
    setSubImage(level, xoffset, yoffset, zoffset, width, height, depth,
                format, GLType_v<PixelT>, pixels); }

  void getImage(
    GLint level, GLenum format, GLenum type, GLsizei bufSize, void* pixels) {
    _tex.getImage(level, format, type, bufSize, pixels); }

  void generateMipmap() { _tex.generateMipmap(); }
    // not valid for GL_TEXTURE_2D_MULTISAMPLE_ARRAY

  inline void clear(GLint level);

  // texture parameters
  void setParameter(GLenum pname, GLfloat param) {
    _tex.setParameterf(pname, param); }
  void setParameter(GLenum pname, GLint param) {
    _tex.setParameteri(pname, param); }
  void setParameterv(GLenum pname, const GLfloat* params) {
    _tex.setParameterfv(pname, params); }
  void setParameterv(GLenum pname, const GLint* params) {
    _tex.setParameteriv(pname, params); }
  void setParameterIv(GLenum pname, const GLint* params) {
    _tex.setParameterIiv(pname, params); }
  void setParameterIv(GLenum pname, const GLuint* params) {
    _tex.setParameterIiuv(pname, params); }

  void getParameterv(GLenum pname, GLfloat* params) {
    _tex.getParameterfv(pname, params); }
  void getParameterv(GLenum pname, GLint* params) {
    _tex.getParameteriv(pname, params); }
  void getParameterIv(GLenum pname, GLint* params) {
    _tex.getParameterIiv(pname, params); }
  void getParameterIv(GLenum pname, GLuint* params) {
    _tex.getParameterIuiv(pname, params); }

  [[nodiscard]] static GLint maxSize() {
    GLint s = 0;
    GX_GLCALL(glGetIntegerv, GLTextureVals<TARGET>::pnameMaxSize, &s);
    return s;
  }

 private:
  GLTextureHandle<VER,TARGET> _tex;
  GLenum _internalformat = GL_NONE;
  GLsizei _levels = 0;
  GLsizei _width = 0;
  GLsizei _height = 0;
  GLsizei _depth = 0;
};


template<int VER>
class gx::GLTextureCubeMap
{
 public:
  using type = GLTextureCubeMap<VER>;

  // operators
  [[nodiscard]] explicit operator bool() const { return _tex.id != 0; }

  // accessors
  [[nodiscard]] static GLenum target() { return GL_TEXTURE_CUBE_MAP; }
  [[nodiscard]] GLuint id() const { return _tex.id; }
  [[nodiscard]] GLenum internalFormat() const { return _internalformat; }
  [[nodiscard]] GLsizei levels() const { return _levels; }
  [[nodiscard]] GLsizei size() const { return _size; } // width/height

  // methods
  inline GLuint init(GLsizei levels, GLenum internalformat, GLsizei size);

  GLuint release() noexcept { return _tex.release(); }
    // releases ownership of managed texture object, returns object id

  void bindUnit(GLuint unit) const { _tex.bindUnit(unit); }

  inline void setSubImage(
    GLint level, GLint xoffset, GLint yoffset, GLuint face,
    GLsizei width, GLsizei height,
    GLenum format, GLenum type, const void* pixels);

  template<typename PixelT>
  void setSubImage(
    GLint level, GLint xoffset, GLint yoffset, GLuint face,
    GLsizei width, GLsizei height, GLenum format, const PixelT* pixels) {
    setSubImage(level, xoffset, yoffset, face, width, height, format,
                GLType_v<PixelT>, pixels); }

  inline void getImage(GLint level, GLuint face, GLenum format, GLenum type,
                       GLsizei bufSize, void* pixels);

  void generateMipmap() { _tex.generateMipmap(); }
    // not valid for GL_TEXTURE_RECTANGLE, GL_TEXTURE_2D_MULTISAMPLE

  inline void clear(GLint level);

  // texture parameters
  void setParameter(GLenum pname, GLfloat param) {
    _tex.setParameterf(pname, param); }
  void setParameter(GLenum pname, GLint param) {
    _tex.setParameteri(pname, param); }
  void setParameterv(GLenum pname, const GLfloat* params) {
    _tex.setParameterfv(pname, params); }
  void setParameterv(GLenum pname, const GLint* params) {
    _tex.setParameteriv(pname, params); }
  void setParameterIv(GLenum pname, const GLint* params) {
    _tex.setParameterIiv(pname, params); }
  void setParameterIv(GLenum pname, const GLuint* params) {
    _tex.setParameterIiuv(pname, params); }

  void getParameterv(GLenum pname, GLfloat* params) {
    _tex.getParameterfv(pname, params); }
  void getParameterv(GLenum pname, GLint* params) {
    _tex.getParameteriv(pname, params); }
  void getParameterIv(GLenum pname, GLint* params) {
    _tex.getParameterIiv(pname, params); }
  void getParameterIv(GLenum pname, GLuint* params) {
    _tex.getParameterIuiv(pname, params); }

  [[nodiscard]] static GLint maxSize() {
    GLint s = 0;
    GX_GLCALL(glGetIntegerv, GLTextureVals<target()>::pnameMaxSize, &s);
    return s;
  }

 private:
  GLTextureHandle<VER,target()> _tex;
  GLenum _internalformat = GL_NONE;
  GLsizei _levels = 0;
  GLsizei _size = 0; // width & height are the same
};


template<int VER>
class gx::GLTextureBuffer
{
 public:
  using type = GLTextureBuffer<VER>;

  // operators
  [[nodiscard]] explicit operator bool() const { return _tex.id != 0; }

  // accessors
  [[nodiscard]] static GLenum target() { return GL_TEXTURE_BUFFER; }
  [[nodiscard]] GLuint id() const { return _tex.id; }
  [[nodiscard]] GLenum internalFormat() const { return _internalformat; }

  // methods
  inline GLuint attachBuffer(GLenum internalformat, GLuint buffer);
  inline void detachBuffer();

  GLuint release() noexcept { return _tex.release(); }
    // releases ownership of managed texture object, returns object id

  void bindUnit(GLuint unit) const { _tex.bindUnit(unit); }

  [[nodiscard]] static GLint maxSize() {
    GLint s = 0;
    GX_GLCALL(glGetIntegerv, GLTextureVals<target()>::pnameMaxSize, &s);
    return s;
  }

 private:
  GLTextureHandle<VER,target()> _tex;
  GLenum _internalformat = GL_NONE;
  // track attached buffer ID?
};


// **** GLTexture1D implementation ****
template<int VER>
GLuint gx::GLTexture1D<VER>::init(
  GLsizei levels, GLenum internalformat, GLsizei width)
{
  _tex.init();
  _internalformat = internalformat;
  _levels = levels;
  _width = width;
  if constexpr (VER < 42) {
    _tex.bindCheck();
    for (int i = 0; i < levels; ++i) {
      GX_GLCALL(glTexImage1D, target(), i, internalformat, width, 0,
                GL_RED, GL_UNSIGNED_BYTE, nullptr);
      width = std::max(1, (width / 2));
    }
  } else if constexpr (VER < 45) {
    _tex.bindCheck();
    GX_GLCALL(glTexStorage1D, target(), levels, internalformat, width);
  } else {
    GX_GLCALL(glTextureStorage1D, _tex, levels, internalformat, width);
  }
  return _tex.id;
}

template<int VER>
void gx::GLTexture1D<VER>::setSubImage(
  GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void* pixels)
{
  GLSetUnpackAlignment(width, format, type);
  if constexpr (VER < 45) {
    _tex.bindCheck();
    GX_GLCALL(glTexSubImage1D, target(), level, xoffset, width, format, type, pixels);
  } else {
    GX_GLCALL(glTextureSubImage1D, _tex.id, level, xoffset, width, format, type, pixels);
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
    setSubImage(level, 0, _width, format, empty.get());
  } else {
    GX_GLCALL(glClearTexImage, _tex.id, level, format, type, nullptr);
  }
}


// **** GLTexture2DT implementation ****
template<int VER, GLenum TARGET>
GLuint gx::GLTexture2DT<VER,TARGET>::init(
  GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)
{
  _tex.init();
  _internalformat = internalformat;
  _levels = levels;
  _width = width;
  _height = height;
  if constexpr (VER < 42) {
    _tex.bindCheck();
    if constexpr (TARGET == GL_TEXTURE_1D_ARRAY
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
    _tex.bindCheck();
    GX_GLCALL(glTexStorage2D, TARGET, levels, internalformat, width, height);
  } else {
    GX_GLCALL(glTextureStorage2D, _tex.id, levels, internalformat, width, height);
  }
  return _tex.id;
}

// TODO: init for GL_TEXTURE_2D_MULTISAMPLE
//   3.3 - glTexImage2DMultisample
//   4.5 - glTextureStorage2DMultisample

template<int VER, GLenum TARGET>
void gx::GLTexture2DT<VER,TARGET>::setSubImage(
  GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
  GLenum format, GLenum type, const void* pixels)
{
  GLSetUnpackAlignment(width, format, type);
  if constexpr (VER < 45) {
    _tex.bindCheck();
    GX_GLCALL(glTexSubImage2D, TARGET, level, xoffset, yoffset,
              width, height, format, type, pixels);
  } else {
    GX_GLCALL(glTextureSubImage2D, _tex.id, level, xoffset, yoffset,
              width, height, format, type, pixels);
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
    setSubImage(level, 0, 0, _width, _height, format, empty.get());
  } else {
    GX_GLCALL(glClearTexImage, _tex.id, level, format, type, nullptr);
  }
}


// **** GLTexture3DT implementation ****
template<int VER, GLenum TARGET>
GLuint gx::GLTexture3DT<VER,TARGET>::init(
  GLsizei levels, GLenum internalformat,
  GLsizei width, GLsizei height, GLsizei depth)
{
  _tex.init();
  _internalformat = internalformat;
  _levels = levels;
  _width = width;
  _height = height;
  _depth = depth;
  if constexpr (VER < 42) {
    _tex.bindCheck();
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
    _tex.bindCheck();
    GX_GLCALL(glTexStorage3D, TARGET, levels, internalformat,
              width, height, depth);
  } else {
    GX_GLCALL(glTextureStorage3D, _tex.id, levels, internalformat,
              width, height, depth);
  }
  return _tex.id;
}

// TODO: init for GL_TEXTURE_2D_MULTISAMPLE_ARRAY
//   3.3 - glTexImage3DMultisample
//   4.5 - glTextureStorage3DMultisample

template<int VER, GLenum TARGET>
void gx::GLTexture3DT<VER,TARGET>::setSubImage(
  GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
  GLsizei width, GLsizei height, GLsizei depth,
  GLenum format, GLenum type, const void* pixels)
{
  GLSetUnpackAlignment(width, format, type);
  if constexpr (VER < 45) {
    _tex.bindCheck();
    GX_GLCALL(glTexSubImage3D, TARGET, level, xoffset, yoffset, zoffset,
              width, height, depth, format, type, pixels);
  } else {
    GX_GLCALL(glTextureSubImage3D, _tex.id, level, xoffset, yoffset, zoffset,
              width, height, depth, format, type, pixels);
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
    setSubImage(level, 0, 0, 0, _width, _height, _depth, format, empty.get());
  } else {
    GX_GLCALL(glClearTexImage, _tex.id, level, format, type, nullptr);
  }
}


// **** GLTextureCubeMap implementation ****
template<int VER>
GLuint gx::GLTextureCubeMap<VER>::init(
  GLsizei levels, GLenum internalformat, GLsizei size)
{
  _tex.init();
  _internalformat = internalformat;
  _levels = levels;
  _size = size;

  if constexpr (VER < 45) {
    _tex.bindCheck();
  } else {
    GX_GLCALL(glBindTexture, target(), _tex.id);
  }

  if constexpr (VER < 42) {
    for (int i = 0; i < levels; ++i) {
      for (unsigned int f = 0; f < 6; ++f) {
        GX_GLCALL(glTexImage2D, GL_TEXTURE_CUBE_MAP_POSITIVE_X + f, i,
                  GLint(internalformat), size, size, 0, GL_RED,
                  GL_UNSIGNED_BYTE, nullptr);
      }
      size = std::max(1, (size / 2));
    }
  } else {
    // NOTE: no direct state access function available (VER >= 45)
    for (unsigned int f = 0; f < 6; ++f) {
      GX_GLCALL(glTexStorage2D, GL_TEXTURE_CUBE_MAP_POSITIVE_X + f, levels,
                internalformat, size, size);
    }
  }
  return _tex.id;
}

template<int VER>
void gx::GLTextureCubeMap<VER>::setSubImage(
  GLint level, GLint xoffset, GLint yoffset, GLuint face, GLsizei width,
  GLsizei height, GLenum format, GLenum type, const void* pixels)
{
  GLSetUnpackAlignment(width, format, type);
  if constexpr (VER < 45) {
    _tex.bindCheck();
    GX_GLCALL(glTexSubImage2D, GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face),
              level, xoffset, yoffset, width, height, format, type, pixels);
  } else {
    GX_GLCALL(glTextureSubImage3D, _tex.id, level, xoffset, yoffset, face,
              width, height, 1, format, type, pixels);
  }
}

template<int VER>
void gx::GLTextureCubeMap<VER>::getImage(
  GLint level, GLuint face, GLenum format, GLenum type,
  GLsizei bufSize, void* pixels)
{
  // single face reading
  if constexpr (VER < 45) {
    _tex.bindCheck();
  } else {
    GX_GLCALL(glBindTexture, target(), _tex.id);
  }

  if constexpr (VER < 45) {
    GLLastTextureBind = 0;
    GX_GLCALL(glGetTexImage, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, level,
              format, type, pixels);
    // FIXME: bufSize not used
  } else {
    GX_GLCALL(glGetnTexImage, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, level,
              format, type, bufSize, pixels);
  }

  // TODO: read all faces at once (will require different call for VER < 45)
  //GX_GLCALL(glGetTextureImage, id, level, format, type, bufSize, pixels);
}

template<int VER>
void gx::GLTextureCubeMap<VER>::clear(GLint level)
{
  const GLenum format = GLBaseFormat(_internalformat);
  const GLenum type = (format == GL_DEPTH_STENCIL)
    ? GL_UNSIGNED_INT_24_8 : GL_UNSIGNED_BYTE;

  if constexpr (VER < 44) {
    const int pixel_size = GLPixelSize(format, type);
    const auto empty = std::make_unique<GLubyte[]>(
      _size * _size * pixel_size);
    for (unsigned int f = 0; f < 6; ++f) {
      setSubImage(level, 0, 0, f, _size, _size, format, empty.get());
    }
  } else {
    GX_GLCALL(glClearTexImage, _tex.id, level, format, type, nullptr);
  }

  // TODO: clear single face
}


// **** GLTextureBuffer implementation ****
template<int VER>
GLuint gx::GLTextureBuffer<VER>::attachBuffer(
  GLenum internalformat, GLuint buffer)
{
  if (!_tex.id) { _tex.init(); }
  _internalformat = internalformat;
  if constexpr (VER < 45) {
    _tex.bindCheck();
    GX_GLCALL(glTexBuffer, target(), internalformat, buffer);
  } else {
    GX_GLCALL(glTextureBuffer, _tex.id, internalformat, buffer);
  }
  return _tex.id;
}

template<int VER>
void gx::GLTextureBuffer<VER>::detachBuffer()
{
  if constexpr (VER < 45) {
    _tex.bindCheck();
    GX_GLCALL(glTexBuffer, target(), _internalformat, 0);
  } else {
    GX_GLCALL(glTextureBuffer, _tex.id, _internalformat, 0);
  }
}


// **** GLTextureHandle definition & implementation ****
// internal implementation class to minimize duplicate code
template<int VER, GLenum TARGET>
struct gx::GLTextureHandle
{
  using type = GLTextureHandle<VER,TARGET>;

  GLuint id = 0;

  GLTextureHandle() = default;
  ~GLTextureHandle() { if (GLInitialized) cleanup(); }

  // prevent copy/assignment
  GLTextureHandle(const type&) = delete;
  type& operator=(const type&) = delete;

  // allow move/move-assign
  GLTextureHandle(type&& t) noexcept : id{t.release()} { }
  type& operator=(type&& t) noexcept { id = t.release(); return *this; }

  GLuint init() {
    cleanup();
    if constexpr (VER < 45) {
      GX_GLCALL(glGenTextures, 1, &id);
    } else {
      GX_GLCALL(glCreateTextures, TARGET, 1, &id);
    }
    return id;
  }

  GLuint release() noexcept { return std::exchange(id, 0); }

  void cleanup() noexcept {
    if (id) {
      if constexpr (VER < 45) {
        if (GLLastTextureBind == id) { GLLastTextureBind = 0; }
      }
      GX_GLCALL(glDeleteTextures, 1, &id);
    }
  }

  static void bindUnit(GLuint unit, GLuint tex) {
    if constexpr (VER < 45) {
      GX_GLCALL(glActiveTexture, GL_TEXTURE0 + unit);
      GX_GLCALL(glBindTexture, TARGET, tex);
      GLLastTextureBind = tex;
    } else {
      GX_GLCALL(glBindTextureUnit, unit, tex);
    }
  }

  void bindUnit(GLuint unit) const { bindUnit(unit, id); }
  static void unbindUnit(GLuint unit) { bindUnit(unit, 0); }

  void bindCheck() {
    // only called for VER < 45
    if (GLLastTextureBind != id) {
      // don't care which unit is active
      GX_GLCALL(glBindTexture, TARGET, id);
      GLLastTextureBind = id;
    }
  }

  void getImage(
    GLint level, GLenum format, GLenum type, GLsizei bufSize, void* pixels) {
    if constexpr (VER < 45) {
      bindCheck();
      GX_GLCALL(glGetTexImage, TARGET, level, format, type, pixels);
      // FIXME: bufSize not used
    } else {
      GX_GLCALL(glGetTextureImage, id, level, format, type, bufSize, pixels);
    }
  }

  void generateMipmap() {
    if constexpr (VER < 45) {
      bindCheck();
      GX_GLCALL(glGenerateMipmap, TARGET);
    } else {
      GX_GLCALL(glGenerateTextureMipmap, id);
    }
  }

  void setParameterf(GLenum pname, GLfloat param) {
    if constexpr (VER < 45) {
      bindCheck();
      GX_GLCALL(glTexParameterf, TARGET, pname, param);
    } else {
      GX_GLCALL(glTextureParameterf, id, pname, param);
    }
  }

  void setParameteri(GLenum pname, GLint param) {
    if constexpr (VER < 45) {
      bindCheck();
      GX_GLCALL(glTexParameteri, TARGET, pname, param);
    } else {
      GX_GLCALL(glTextureParameteri, id, pname, param);
    }
  }

  void setParameterfv(GLenum pname, const GLfloat* params) {
    if constexpr (VER < 45) {
      bindCheck();
      GX_GLCALL(glTexParameterfv, TARGET, pname, params);
    } else {
      GX_GLCALL(glTextureParameterfv, id, pname, params);
    }
  }

  void setParameteriv(GLenum pname, const GLint* params) {
    if constexpr (VER < 45) {
      bindCheck();
      GX_GLCALL(glTexParameteriv, TARGET, pname, params);
    } else {
      GX_GLCALL(glTextureParameteriv, id, pname, params);
    }
  }

  void setParameterIiv(GLenum pname, const GLint* params) {
    if constexpr (VER < 45) {
      bindCheck();
      GX_GLCALL(glTexParameterIiv, TARGET, pname, params);
    } else {
      GX_GLCALL(glTextureParameterIiv, id, pname, params);
    }
  }

  void setParameterIuiv(GLenum pname, const GLuint* params) {
    if constexpr (VER < 45) {
      bindCheck();
      GX_GLCALL(glTexParameterIuiv, TARGET, pname, params);
    } else {
      GX_GLCALL(glTextureParameterIuiv, id, pname, params);
    }
  }

  void getParameterfv(GLenum pname, GLfloat* params) {
    if constexpr (VER < 45) {
      bindCheck();
      GX_GLCALL(glGetTexParameterfv, TARGET, pname, params);
    } else {
      GX_GLCALL(glGetTextureParameterfv, id, pname, params);
    }
  }

  void getParameteriv(GLenum pname, GLint* params) {
    if constexpr (VER < 45) {
      bindCheck();
      GX_GLCALL(glGetTexParameteriv, TARGET, pname, params);
    } else {
      GX_GLCALL(glGetTextureParameteriv, id, pname, params);
    }
  }

  void getParameterIiv(GLenum pname, GLint* params) {
    if constexpr (VER < 45) {
      bindCheck();
      GX_GLCALL(glGetTexParameterIiv, TARGET, pname, params);
    } else {
      GX_GLCALL(glGetTextureParameterIiv, id, pname, params);
    }
  }

  void getParameterIuiv(GLenum pname, GLuint* params) {
    if constexpr (VER < 45) {
      bindCheck();
      GX_GLCALL(glGetTexParameterIuiv, TARGET, pname, params);
    } else {
      GX_GLCALL(glGetTextureParameterIuiv, id, pname, params);
    }
  }
};
