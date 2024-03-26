//
// gx/GLProgram.hh
// Copyright (C) 2024 Richard Bradley
//
// wrapper for OpenGL program object
//

#pragma once
#include "GLShader.hh"
#include "OpenGL.hh"
#include <utility>
#include <string>

namespace gx {
  class GLProgram;
}

class gx::GLProgram
{
 public:
  GLProgram() = default;
  ~GLProgram() { if (GLVersion != 0) cleanup(); }

  // prevent copy/assignment
  GLProgram(const GLProgram&) = delete;
  GLProgram& operator=(const GLProgram&) = delete;

  // enable move
  inline GLProgram(GLProgram&& p) noexcept;
  inline GLProgram& operator=(GLProgram&& p) noexcept;

  // operators
  [[nodiscard]] explicit operator bool() const { return _prog; }

  // accessors
  [[nodiscard]] GLuint id() const { return _prog; }

  // methods
  inline GLuint init();
    // creates program object - only call after GL context creation

  GLuint release() noexcept { return std::exchange(_prog, 0); }
    // releases ownership of managed program object, returns object id

  void attach(GLuint shader) { GX_GLCALL(glAttachShader, _prog, shader); }
  void attach(const GLShader& s) { attach(s.id()); }

  void detach(GLuint shader) { GX_GLCALL(glDetachShader, _prog, shader); }
  void detach(const GLShader& s) { detach(s.id()); }

  void bindAttribLocation(GLuint index, const char* name) {
    GX_GLCALL(glBindAttribLocation, _prog, index, name); }

  inline bool link();
  [[nodiscard]] inline bool validate();
  [[nodiscard]] inline std::string infoLog() const;

  [[nodiscard]] inline GLint getAttribLocation(const char* name) const;
  [[nodiscard]] inline GLint getUniformLocation(const char* name) const;
  [[nodiscard]] inline GLuint getUniformBlockIndex(const char* blockName) const;

  void setUniformBlockBinding(GLuint blockIndex, GLuint blockBinding) {
    GX_GLCALL(glUniformBlockBinding, _prog, blockIndex, blockBinding); }
  void setUniformBlockBinding(const char* blockName, GLuint blockBinding) {
    setUniformBlockBinding(getUniformBlockIndex(blockName), blockBinding); }

  void use() { GX_GLCALL(glUseProgram, _prog); }

  // variable arg overloads
  template<typename... Args>
  bool link(const Args&... args) {
    (attach(args),...);
    const bool status = link();
    (detach(args),...);
    return status;
  }

  template<typename... Args>
  GLuint init(const Args&... args) {
    return (init() && link(args...)) ? _prog : 0;
  }

 private:
  GLuint _prog = 0;

  inline void cleanup() noexcept;
};


// **** Inline Implementations ****
gx::GLProgram::GLProgram(GLProgram&& p) noexcept
  : _prog{p.release()} { }

gx::GLProgram& gx::GLProgram::operator=(GLProgram&& p) noexcept
{
  if (this != &p) {
    cleanup();
    _prog = p.release();
  }
  return *this;
}

GLuint gx::GLProgram::init()
{
  cleanup();
  _prog = glCreateProgram();
  return _prog;
}

bool gx::GLProgram::link()
{
  GX_GLCALL(glLinkProgram, _prog);

  GLint status = 0;
  GX_GLCALL(glGetProgramiv, _prog, GL_LINK_STATUS, &status);
  return status;
}

bool gx::GLProgram::validate()
{
  GX_GLCALL(glValidateProgram, _prog);

  GLint status = 0;
  GX_GLCALL(glGetProgramiv, _prog, GL_VALIDATE_STATUS, &status);
  return status;
}

std::string gx::GLProgram::infoLog() const
{
  GLint logLen = 0;
  GX_GLCALL(glGetProgramiv, _prog, GL_INFO_LOG_LENGTH, &logLen);
  if (logLen <= 0) { return {}; }

  GLsizei len = 0;
  auto tmp = std::make_unique<char[]>(std::size_t(logLen));
  //char* tmp = (char*)__builtin_alloca(logLen);
  //char tmp[logLen];
  GX_GLCALL(glGetProgramInfoLog, _prog, logLen, &len, tmp.get());

  return {tmp.get(), std::size_t(len)};
}

GLint gx::GLProgram::getAttribLocation(const char* name) const
{
  const auto loc = glGetAttribLocation(_prog, name);
  #ifdef GX_DEBUG_GL
  GLCheckErrors("glGetAttribLocation");
  #endif
  // NOTE: returns -1 if name is not an active attribute
  return loc;
}

GLint gx::GLProgram::getUniformLocation(const char* name) const
{
  const auto loc = glGetUniformLocation(_prog, name);
  #ifdef GX_DEBUG_GL
  GLCheckErrors("glGetUniformLocation");
  #endif
  // NOTE: returns -1 if name not found
  return loc;
}

GLuint gx::GLProgram::getUniformBlockIndex(const char* blockName) const
{
  const auto index = glGetUniformBlockIndex(_prog, blockName);
  #ifdef GX_DEBUG_GL
  GLCheckErrors("glGetUniformBlockIndex");
  #endif
  // NOTE: returns GL_INVALID_INDEX on error
  return index;
}

void gx::GLProgram::cleanup() noexcept
{
  if (_prog) {
    GX_GLCALL(glDeleteProgram, _prog);
  }
}
