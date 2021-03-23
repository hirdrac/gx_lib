//
// gx/GLShader.hh
// Copyright (C) 2021 Richard Bradley
//
// wrapper for OpenGL shader object
//

#pragma once
#include "OpenGL.hh"
#include <string>
#include <utility>
//#include <memory>


inline namespace GX_GLNAMESPACE {

class GLShader
{
 public:
  GLShader() = default;
  inline GLShader(GLShader&& s) noexcept;
  ~GLShader() { if (GLInitialized) cleanup(); }

  // operators
  inline GLShader& operator=(GLShader&& s) noexcept;
  [[nodiscard]] explicit operator bool() const { return _shader; }

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
    GX_GLCALL(glShaderSource, _shader, 2, src_array, nullptr);
  } else {
    GX_GLCALL(glShaderSource, _shader, 1, &src, nullptr);
  }

  GX_GLCALL(glCompileShader, _shader);
  int compile_ok = GL_FALSE;
  GX_GLCALL(glGetShaderiv, _shader, GL_COMPILE_STATUS, &compile_ok);

  return compile_ok ? _shader : 0;
}

std::string GLShader::infoLog()
{
  GLint logLen = 0;
  GX_GLCALL(glGetShaderiv, _shader, GL_INFO_LOG_LENGTH, &logLen);
  if (logLen <= 0) { return std::string(); }

  GLsizei len = 0;
  //auto tmp = std::make_unique<char[]>(logLen);
  //char* tmp = (char*)__builtin_alloca(logLen);
  char tmp[logLen];
  GX_GLCALL(glGetShaderInfoLog, _shader, logLen, &len, tmp);

  return std::string(tmp, std::size_t(len));
}

void GLShader::cleanup() noexcept
{
  if (_shader) {
    GX_GLCALL(glDeleteShader, _shader);
  }
}

} // end GX_GLNAMESPACE
