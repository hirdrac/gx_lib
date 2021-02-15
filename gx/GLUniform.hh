//
// gx/GLUniform.hh
// Copyright (C) 2021 Richard Bradley
//

#pragma once
#include "OpenGL.hh"


inline namespace GX_GLNAMESPACE {

class GLUniform1i
{
 public:
  GLUniform1i() = default;
  GLUniform1i(GLint loc) : _loc(loc) { }

  explicit operator bool() const { return _loc != -1; }

  void set(GLint v) { GX_GLCALL(glUniform1i, _loc, v); }

 private:
  GLint _loc = -1;
};

class GLUniform2i
{
 public:
  GLUniform2i() = default;
  GLUniform2i(GLint loc) : _loc(loc) { }

  explicit operator bool() const { return _loc != -1; }

  void set(GLfloat v0, GLfloat v1) { GX_GLCALL(glUniform2i, _loc, v0, v1); }

  template <class T>
  void set(const T& v) { GX_GLCALL(glUniform2iv, _loc, 1, std::data(v)); }

 private:
  GLint _loc = -1;
};

class GLUniform3i
{
 public:
  GLUniform3i() = default;
  GLUniform3i(GLint loc) : _loc(loc) { }

  explicit operator bool() const { return _loc != -1; }

  void set(GLfloat v0, GLfloat v1, GLfloat v2) {
    GX_GLCALL(glUniform3i, _loc, v0, v1, v2); }

  template <class T>
  void set(const T& v) { GX_GLCALL(glUniform3iv, _loc, 1, std::data(v)); }

 private:
  GLint _loc = -1;
};

class GLUniform4i
{
 public:
  GLUniform4i() = default;
  GLUniform4i(GLint loc) : _loc(loc) { }

  explicit operator bool() const { return _loc != -1; }

  void set(GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
    GX_GLCALL(glUniform4i, _loc, v0, v1, v2, v3); }

  template <class T>
  void set(const T& v) { GX_GLCALL(glUniform4iv, _loc, 1, std::data(v)); }

 private:
  GLint _loc = -1;
};

class GLUniform1f
{
 public:
  GLUniform1f() = default;
  GLUniform1f(GLint loc) : _loc(loc) { }

  explicit operator bool() const { return _loc != -1; }

  void set(GLint v) { GX_GLCALL(glUniform1f, _loc, v); }

 private:
  GLint _loc = -1;
};

class GLUniform2f
{
 public:
  GLUniform2f() = default;
  GLUniform2f(GLint loc) : _loc(loc) { }

  explicit operator bool() const { return _loc != -1; }

  void set(GLfloat v0, GLfloat v1) { GX_GLCALL(glUniform2f, _loc, v0, v1); }

  template <class T>
  void set(const T& v) { GX_GLCALL(glUniform2fv, _loc, 1, std::data(v)); }

 private:
  GLint _loc = -1;
};

class GLUniform3f
{
 public:
  GLUniform3f() = default;
  GLUniform3f(GLint loc) : _loc(loc) { }

  explicit operator bool() const { return _loc != -1; }

  void set(GLfloat v0, GLfloat v1, GLfloat v2) {
    GX_GLCALL(glUniform3f, _loc, v0, v1, v2); }

  template <class T>
  void set(const T& v) { GX_GLCALL(glUniform3fv, _loc, 1, std::data(v)); }

 private:
  GLint _loc = -1;
};

class GLUniform4f
{
 public:
  GLUniform4f() = default;
  GLUniform4f(GLint loc) : _loc(loc) { }

  explicit operator bool() const { return _loc != -1; }

  void set(GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
    GX_GLCALL(glUniform4f, _loc, v0, v1, v2, v3); }

  template <class T>
  void set(const T& v) { GX_GLCALL(glUniform4fv, _loc, 1, std::data(v)); }

 private:
  GLint _loc = -1;
};

class GLUniformMat4f
{
 public:
  GLUniformMat4f() = default;
  GLUniformMat4f(GLint loc) : _loc(loc) { }

  explicit operator bool() const { return _loc != -1; }

  template <class T>
  void set(const T& m, bool transpose = false) {
    GX_GLCALL(glUniformMatrix4fv, _loc, 1, GLBool(transpose), std::data(m)); }

 private:
  GLint _loc = -1;
};

} // end GX_GLNAMESPACE
