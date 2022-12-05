//
// gx/OpenGL.hh
// Copyright (C) 2022 Richard Bradley
//
// OpenGL API include & generic utility functions
//

#pragma once
#include "3rd/glad.h"
#include <string>
#include <string_view>

namespace gx {

// **** Globals ****
inline bool GLInitialized = false;
  // check if GL calls are safe (mainly for destructors)

inline GLuint GLLastArrayBufferBind = 0;
inline GLuint GLLastCopyWriteBufferBind = 0;
inline GLuint GLLastVertexArrayBind = 0;
inline GLuint GLLastTextureBind = 0;
  // cache bind values for auto-binding in OpenGL versions without
  // direct state access methods (GL < 4.5)


// **** Functions ****
bool GLSetupContext(GLADloadproc loadProc);
  // call after GL context creation to setup context
  // returns true on success

void GLClearState();
  // call after every frame to reset GL state

int GLCheckErrors(std::string_view msg, const char* file = __FILE__,
		  int line = __LINE__);
  // check for and log all OpenGL errors
  // returns number of errors found

[[nodiscard]] std::string GLErrorStr(GLenum error);
  // returns error string (return value of glGetError)

[[nodiscard]] constexpr int GLTypeSize(GLenum type)
{
  switch (type) {
    case GL_BYTE:
    case GL_UNSIGNED_BYTE:
    case GL_UNSIGNED_BYTE_3_3_2:
    case GL_UNSIGNED_BYTE_2_3_3_REV:
      return 1;
    case GL_SHORT:
    case GL_UNSIGNED_SHORT:
    case GL_UNSIGNED_SHORT_4_4_4_4:
    case GL_UNSIGNED_SHORT_5_5_5_1:
    case GL_UNSIGNED_SHORT_5_6_5:
    case GL_UNSIGNED_SHORT_5_6_5_REV:
    case GL_UNSIGNED_SHORT_4_4_4_4_REV:
    case GL_UNSIGNED_SHORT_1_5_5_5_REV:
    case GL_HALF_FLOAT:
      return 2;
    case GL_INT:
    case GL_INT_2_10_10_10_REV:
    case GL_UNSIGNED_INT:
    case GL_UNSIGNED_INT_8_8_8_8:
    case GL_UNSIGNED_INT_8_8_8_8_REV:
    case GL_UNSIGNED_INT_10_10_10_2:
    case GL_UNSIGNED_INT_2_10_10_10_REV:
    case GL_UNSIGNED_INT_10F_11F_11F_REV:
    case GL_UNSIGNED_INT_5_9_9_9_REV:
    case GL_UNSIGNED_INT_24_8:
    case GL_FLOAT:
    case GL_FIXED:
      return 4;
    case GL_DOUBLE:
      return 8;
    default:
      return 0;
  }
}

[[nodiscard]] constexpr int GLPixelSize(GLenum format, GLenum type)
{
  int s = GLTypeSize(type);
  if (type == GL_UNSIGNED_BYTE || type == GL_BYTE || type == GL_UNSIGNED_SHORT
      || type == GL_SHORT || type == GL_UNSIGNED_INT || type == GL_INT
      || type == GL_FLOAT)
  {
    switch (format) {
      case GL_RG:
        s *= 2; break;

      case GL_RGB:
      case GL_BGR:
        s *= 3; break;

      case GL_RGBA:
      case GL_BGRA:
        s *= 4; break;

      default:
        // GL_RED, GL_DEPTH_COMPONENT, GL_DEPTH_STENCIL, GL_STENCIL_INDEX
        break;
    }
  }
  return s;
}

[[nodiscard]] constexpr GLenum GLFormat(int channels)
{
  switch (channels) {
    case 1:  return GL_RED;
    case 2:  return GL_RG;
    case 3:  return GL_RGB;
    case 4:  return GL_RGBA;
    default: return GL_NONE;
  }
}

[[nodiscard]] constexpr GLenum GLBaseFormat(GLenum internalformat)
{
  switch (internalformat) {
    case GL_DEPTH_COMPONENT:
    case GL_DEPTH_COMPONENT16:
    case GL_DEPTH_COMPONENT24:
    case GL_DEPTH_COMPONENT32:
    case GL_DEPTH_COMPONENT32F:
      return GL_DEPTH_COMPONENT;

    case GL_DEPTH_STENCIL:
    case GL_DEPTH24_STENCIL8:
    case GL_DEPTH32F_STENCIL8:
      return GL_DEPTH_STENCIL;

    case GL_STENCIL_INDEX:
    case GL_STENCIL_INDEX1:
    case GL_STENCIL_INDEX4:
    case GL_STENCIL_INDEX8:
    case GL_STENCIL_INDEX16:
      return GL_STENCIL_INDEX;

    case GL_RED:
    case GL_R8:       case GL_R8_SNORM:
    case GL_R8I:      case GL_R8UI:
    case GL_R16:      case GL_R16_SNORM:
    case GL_R16I:     case GL_R16UI:
    case GL_R16F:
    case GL_R32I:     case GL_R32UI:
    case GL_R32F:
    case GL_COMPRESSED_RED:
    case GL_COMPRESSED_RED_RGTC1:
    case GL_COMPRESSED_SIGNED_RED_RGTC1:
    case GL_COMPRESSED_R11_EAC:
    case GL_COMPRESSED_SIGNED_R11_EAC:
      return GL_RED;

    case GL_RG:
    case GL_RG8:      case GL_RG8_SNORM:
    case GL_RG8I:     case GL_RG8UI:
    case GL_RG16:     case GL_RG16_SNORM:
    case GL_RG16I:    case GL_RG16UI:
    case GL_RG16F:
    case GL_RG32I:    case GL_RG32UI:
    case GL_RG32F:
    case GL_COMPRESSED_RG:
    case GL_COMPRESSED_RG_RGTC2:
    case GL_COMPRESSED_SIGNED_RG_RGTC2:
    case GL_COMPRESSED_RG11_EAC:
    case GL_COMPRESSED_SIGNED_RG11_EAC:
      return GL_RG;

    case GL_RGB:
    case GL_R3_G3_B2:
    case GL_RGB4:
    case GL_RGB5:
    case GL_RGB8:     case GL_RGB8_SNORM:
    case GL_RGB8I:    case GL_RGB8UI:
    case GL_RGB10:
    case GL_RGB12:
    case GL_RGB16:    case GL_RGB16_SNORM:
    case GL_RGB16I:   case GL_RGB16UI:
    case GL_RGB16F:
    case GL_RGB32I:   case GL_RGB32UI:
    case GL_RGB32F:
    case GL_R11F_G11F_B10F:
    case GL_RGB9_E5:
    case GL_SRGB:
    case GL_SRGB8:
    case GL_COMPRESSED_RGB:
    case GL_COMPRESSED_SRGB:
    case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT:
    case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT:
    case GL_COMPRESSED_RGB8_ETC2:
    case GL_COMPRESSED_SRGB8_ETC2:
      return GL_RGB;

    case GL_RGBA:
    case GL_RGBA2:
    case GL_RGBA4:
    case GL_RGB5_A1:
    case GL_RGBA8:    case GL_RGBA8_SNORM:
    case GL_RGBA8I:   case GL_RGBA8UI:
    case GL_RGB10_A2: case GL_RGB10_A2UI:
    case GL_RGBA12:
    case GL_RGBA16:
    case GL_RGBA16I:  case GL_RGBA16UI:
    case GL_RGBA16F:
    case GL_RGBA32I:  case GL_RGBA32UI:
    case GL_RGBA32F:
    case GL_SRGB_ALPHA:
    case GL_SRGB8_ALPHA8:
    case GL_COMPRESSED_RGBA:
    case GL_COMPRESSED_SRGB_ALPHA:
    case GL_COMPRESSED_RGBA_BPTC_UNORM:
    case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
    case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
    case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
    case GL_COMPRESSED_RGBA8_ETC2_EAC:
    case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
      return GL_RGBA;

    default:
      return GL_NONE;
  }
}


// **** Types ****
template<class T> struct GLType { };

template<> struct GLType<GLfloat> {
  static constexpr GLenum value = GL_FLOAT; };

template<> struct GLType<GLdouble> {
  static constexpr GLenum value = GL_DOUBLE; };

template<> struct GLType<GLbyte> {
  static constexpr GLenum value = GL_BYTE; };

template<> struct GLType<GLubyte> {
  static constexpr GLenum value = GL_UNSIGNED_BYTE; };

template<> struct GLType<GLshort> {
  static constexpr GLenum value = GL_SHORT; };

template<> struct GLType<GLushort> {
  static constexpr GLenum value = GL_UNSIGNED_SHORT; };

template<> struct GLType<GLint> {
  static constexpr GLenum value = GL_INT; };

template<> struct GLType<GLuint> {
  static constexpr GLenum value = GL_UNSIGNED_INT; };

template<class T>
constexpr GLenum GLType_v = GLType<T>::value;

} // end gx namespace


// **** Macros ****
#ifdef GX_DEBUG_GL
#define GX_GLCALL(fn,...) do { (fn)(__VA_ARGS__); gx::GLCheckErrors(#fn); } while(0)
#else
#define GX_GLCALL(fn,...) (fn)(__VA_ARGS__)
#endif
