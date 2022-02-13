//
// gx/GLProgram.hh
// Copyright (C) 2022 Richard Bradley
//
// wrapper for OpenGL program object
//

#pragma once
#include "GLShader.hh"
#include "OpenGL.hh"
#include <string>
#include <utility>
//#include <memory>


inline namespace GX_GLNAMESPACE {

class GLProgram
{
 public:
  GLProgram() = default;
  ~GLProgram() { if (GLInitialized) cleanup(); }

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

  [[nodiscard]] GLint getAttribLocation(const char* name) const {
    return glGetAttribLocation(_prog, name); }
  [[nodiscard]] GLint getUniformLocation(const char* name) const {
    return glGetUniformLocation(_prog, name); }

  [[nodiscard]] GLuint getUniformBlockIndex(const char* blockName) const {
    return glGetUniformBlockIndex(_prog, blockName); }
  void setUniformBlockBinding(GLuint blockIndex, GLuint blockBinding) {
    GX_GLCALL(glUniformBlockBinding, _prog, blockIndex, blockBinding); }
  void setUniformBlockBinding(const char* blockName, GLuint blockBinding) {
    setUniformBlockBinding(getUniformBlockIndex(blockName), blockBinding); }

  void use() { GX_GLCALL(glUseProgram, _prog); }

  // variable arg overloads
  template<typename... Args>
  bool link(const Args&... args) {
    (attach(args),...);
    bool status = link();
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
GLProgram::GLProgram(GLProgram&& p) noexcept
  : _prog{p.release()} { }

GLProgram& GLProgram::operator=(GLProgram&& p) noexcept
{
  if (this != &p) {
    cleanup();
    _prog = p.release();
  }
  return *this;
}

GLuint GLProgram::init()
{
  cleanup();
  _prog = glCreateProgram();
  return _prog;
}

bool GLProgram::link()
{
  GX_GLCALL(glLinkProgram, _prog);

  GLint status = 0;
  GX_GLCALL(glGetProgramiv, _prog, GL_LINK_STATUS, &status);
  return status;
}

bool GLProgram::validate()
{
  GX_GLCALL(glValidateProgram, _prog);

  GLint status = 0;
  GX_GLCALL(glGetProgramiv, _prog, GL_VALIDATE_STATUS, &status);
  return status;
}

std::string GLProgram::infoLog() const
{
  GLint logLen = 0;
  GX_GLCALL(glGetProgramiv, _prog, GL_INFO_LOG_LENGTH, &logLen);
  if (logLen <= 0) { return std::string(); }

  GLsizei len = 0;
  //auto tmp = std::make_unique<char[]>(logLen);
  //char* tmp = (char*)__builtin_alloca(logLen);
  char tmp[logLen];
  GX_GLCALL(glGetProgramInfoLog, _prog, logLen, &len, tmp);

  return std::string(tmp, std::size_t(len));
}

void GLProgram::cleanup() noexcept
{
  if (_prog) {
    GX_GLCALL(glDeleteProgram, _prog);
  }
}

} // end GX_GLNAMESPACE
