//
// gx/GLShader.hh
// Copyright (C) 2021 Richard Bradley
//
// wrapper for OpenGL shader object
//

#pragma once
#include "OpenGL.hh"
#include <string>
#include <memory>
#include <utility>


inline namespace GX_GLNAMESPACE {

class GLShader
{
 public:
  GLShader() = default;
  inline GLShader(GLShader&& s) noexcept;
  ~GLShader() { if (GLInitialized) cleanup(); }

  // operators
  inline GLShader& operator=(GLShader&& s) noexcept;
  explicit operator bool() const { return _shader; }

  // accessors
  [[nodiscard]] GLuint id() const { return _shader; }

  // methods
  inline GLuint init(
    GLenum type, const char* src, const char* header = nullptr);
  GLuint release() noexcept { return std::exchange(_shader, 0); }
  [[nodiscard]] inline std::string infoLog();

 private:
  GLuint _shader = 0;

  inline void cleanup() noexcept;

  // prevent copy/assignment
  GLShader(const GLShader&) = delete;
  GLShader& operator=(const GLShader&) = delete;
};


// **** Inline Implementations ****
GLShader::GLShader(GLShader&& s) noexcept
  : _shader(s.release()) { }

GLShader& GLShader::operator=(GLShader&& s) noexcept
{
  if (this != &s) {
    cleanup();
    _shader = s.release();
  }
  return *this;
}

GLuint GLShader::init(GLenum type, const char* src, const char* header)
{
  cleanup();
  _shader = glCreateShader(type);
  if (!_shader) { return 0; }

  if (header) {
    const char* src_array[] = { header, src };
    GLCALL(glShaderSource, _shader, 2, src_array, nullptr);
  } else {
    GLCALL(glShaderSource, _shader, 1, &src, nullptr);
  }

  GLCALL(glCompileShader, _shader);
  int compile_ok = GL_FALSE;
  GLCALL(glGetShaderiv, _shader, GL_COMPILE_STATUS, &compile_ok);

  return compile_ok ? _shader : 0;
}

std::string GLShader::infoLog()
{
  GLint logLen = 0;
  GLCALL(glGetShaderiv, _shader, GL_INFO_LOG_LENGTH, &logLen);
  if (logLen <= 0) { return std::string(); }

  GLsizei len = 0;
  auto tmp = std::make_unique<char[]>(logLen);
  GLCALL(glGetShaderInfoLog, _shader, logLen, &len, tmp.get());

  return std::string(tmp.get(), len);
}

void GLShader::cleanup() noexcept
{
  if (_shader) {
    GLCALL(glDeleteShader, _shader);
  }
}

} // end GX_GLNAMESPACE
