//
// gx/OpenGL.hh
// Copyright (C) 2021 Richard Bradley
//
// OpenGL API include & generic utility functions
//

#pragma once
#include "3rd/glad.h"
#include <string>
#include <string_view>

#ifdef GX_GL33
#define GLNAMESPACE gx_gl33
constexpr int GL_VERSION_MAJOR = 3;
constexpr int GL_VERSION_MINOR = 3;
constexpr const char* GLSL_SOURCE_HEADER =
  "#version 330 core\n"
  "#extension GL_ARB_shading_language_packing : enable\n";
#else
#define GLNAMESPACE gx_gl45
constexpr int GL_VERSION_MAJOR = 4;
constexpr int GL_VERSION_MINOR = 5;
constexpr const char* GLSL_SOURCE_HEADER =
  "#version 450 core\n";
#endif


inline namespace GLNAMESPACE {

// **** Globals ****
extern bool GLInitialized;
  // check if GL calls are safe (mainly for destructors)

#ifdef GX_GL33
extern GLuint GLLastBufferBind;
extern GLuint GLLastArrayBufferBind;
extern GLuint GLLastVertexArrayBind;
extern GLuint GLLastTextureBind;
  // cache bind values for auto-binding in OpenGL versions without
  // direct state access methods (GL < 4.5)
#endif


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

[[nodiscard]] constexpr GLboolean GLBool(bool val) {
  return val ? GL_TRUE : GL_FALSE; }

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
  if (type == GL_UNSIGNED_BYTE || type == GL_BYTE
      || type == GL_UNSIGNED_SHORT || type == GL_SHORT
      || type == GL_UNSIGNED_INT || type == GL_INT
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
        break;
    }
  }
  return s;
}


// **** Macros ****
#ifdef GX_DEBUG_GL
#define GLCALL(fn,...) do { (fn)(__VA_ARGS__); GLCheckErrors(#fn); } while(0)
#else
#define GLCALL(fn,...) (fn)(__VA_ARGS__)
#endif


// **** Types ****
template <typename T> struct GLType { };

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

template <class T>
constexpr GLenum GLType_v = GLType<T>::value;

} // end GLNAMESPACE
