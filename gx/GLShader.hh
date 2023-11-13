//
// gx/GLShader.hh
// Copyright (C) 2023 Richard Bradley
//
// wrapper for OpenGL shader object
//

#pragma once
#include "OpenGL.hh"
#include <utility>
#include <string>

namespace gx {
  class GLShader;
}

class gx::GLShader
{
 public:
  GLShader() = default;
  ~GLShader() { if (GLInitialized) cleanup(); }

  // prevent copy/assignment
  GLShader(const GLShader&) = delete;
  GLShader& operator=(const GLShader&) = delete;

  // enable move
  inline GLShader(GLShader&& s) noexcept;
  inline GLShader& operator=(GLShader&& s) noexcept;

  // operators
  [[nodiscard]] explicit operator bool() const { return _shader; }

  // accessors
  [[nodiscard]] GLuint id() const { return _shader; }

  // methods
  template<typename... SrcArgs>
  inline GLuint init(GLenum type, SrcArgs... src);

  GLuint release() noexcept { return std::exchange(_shader, 0); }
  [[nodiscard]] inline std::string infoLog();

 private:
  GLuint _shader = 0;

  inline void cleanup() noexcept;
};


// **** Inline Implementations ****
gx::GLShader::GLShader(GLShader&& s) noexcept
  : _shader{s.release()} { }

gx::GLShader& gx::GLShader::operator=(GLShader&& s) noexcept
{
  if (this != &s) {
    cleanup();
    _shader = s.release();
  }
  return *this;
}

template<typename... SrcArgs>
GLuint gx::GLShader::init(GLenum type, SrcArgs... src)
{
  cleanup();
  _shader = glCreateShader(type);
  if (!_shader) { return 0; }

  const char* src_array[] = { src... };
  GX_GLCALL(glShaderSource, _shader, sizeof...(src), src_array, nullptr);
  GX_GLCALL(glCompileShader, _shader);
  int compile_ok = GL_FALSE;
  GX_GLCALL(glGetShaderiv, _shader, GL_COMPILE_STATUS, &compile_ok);

  return compile_ok ? _shader : 0;
}

std::string gx::GLShader::infoLog()
{
  GLint logLen = 0;
  GX_GLCALL(glGetShaderiv, _shader, GL_INFO_LOG_LENGTH, &logLen);
  if (logLen <= 0) { return {}; }

  GLsizei len = 0;
  //auto tmp = std::make_unique<char[]>(logLen);
  //char* tmp = (char*)__builtin_alloca(logLen);
  char tmp[logLen];
  GX_GLCALL(glGetShaderInfoLog, _shader, logLen, &len, tmp);

  return {tmp, std::size_t(len)};
}

void gx::GLShader::cleanup() noexcept
{
  if (_shader) {
    GX_GLCALL(glDeleteShader, _shader);
  }
}
