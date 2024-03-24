//
// gx/GLVertexArray.hh
// Copyright (C) 2024 Richard Bradley
//
// wrapper for OpenGL vertex array object
//

#pragma once
#include "GLBuffer.hh"
#include "OpenGL.hh"
#include <utility>

namespace gx {
  template<int VER> class GLVertexArray;
}

template<int VER>
class gx::GLVertexArray
{
 public:
  using type = GLVertexArray<VER>;

  GLVertexArray() = default;
  ~GLVertexArray() { if (GLVersion != 0) cleanup(); }

  // prevent copy/assignment
  GLVertexArray(const type&) = delete;
  type& operator=(const type&) = delete;

  // enable move
  inline GLVertexArray(type&& v) noexcept;
  inline type& operator=(type&& v) noexcept;

  // operators
  [[nodiscard]] explicit operator bool() const { return _vao; }

  // accessors
  [[nodiscard]] GLuint id() const { return _vao; }

  // methods
  inline GLuint init();
  GLuint release() noexcept { return std::exchange(_vao, 0); }

  inline void bind();
  inline static void unbind();

  inline void enableAttrib(GLuint index);
  inline void disableAttrib(GLuint index);
  inline void setAttrib(
    GLuint index, GLBuffer<VER>& buffer, GLintptr offset,
    GLsizei stride, GLint size, GLenum type, GLboolean normalized);
  inline void setAttribI(
    GLuint index, GLBuffer<VER>& buffer, GLintptr offset,
    GLsizei stride, GLint size, GLenum type);
  inline void setAttribL(
    GLuint index, GLBuffer<VER>& buffer, GLintptr offset,
    GLsizei stride, GLint size, GLenum type);
  inline void setAttribDivisor(GLuint index, GLuint divisor);

 private:
  GLuint _vao = 0;

  void bindCheck() {
    if (GLLastVertexArrayBind != _vao) { bind(); }
  }

  void bufferBindCheck(GLBuffer<VER>& buffer) {
    if (GLLastArrayBufferBind != buffer.id()) {
      buffer.bind(GL_ARRAY_BUFFER);
    }
  }

  inline void cleanup() noexcept;
};


// **** Inline Implementation ****
template<int VER>
gx::GLVertexArray<VER>::GLVertexArray(GLVertexArray&& v) noexcept
  : _vao{v.release()} { }

template<int VER>
gx::GLVertexArray<VER>& gx::GLVertexArray<VER>::operator=(GLVertexArray&& v) noexcept
{
  if (this != &v) {
    cleanup();
    _vao = v.release();
  }
  return *this;
}

template<int VER>
GLuint gx::GLVertexArray<VER>::init()
{
  cleanup();
  if constexpr (VER < 45) {
    GX_GLCALL(glGenVertexArrays, 1, &_vao);
    // VA created when bound for the first time
  } else {
    GX_GLCALL(glCreateVertexArrays, 1, &_vao);
  }
  return _vao;
}

template<int VER>
void gx::GLVertexArray<VER>::bind()
{
  GX_GLCALL(glBindVertexArray, _vao);
  if constexpr (VER < 45) { GLLastVertexArrayBind = _vao; }
}

template<int VER>
void gx::GLVertexArray<VER>::unbind()
{
  GX_GLCALL(glBindVertexArray, 0);
  if constexpr (VER < 45) { GLLastVertexArrayBind = 0; }
}

template<int VER>
void gx::GLVertexArray<VER>::enableAttrib(GLuint index)
{
  if constexpr (VER < 45) {
    bindCheck();
    GX_GLCALL(glEnableVertexAttribArray, index);
  } else {
    GX_GLCALL(glEnableVertexArrayAttrib, _vao, index);
  }
}

template<int VER>
void gx::GLVertexArray<VER>::disableAttrib(GLuint index)
{
  if constexpr (VER < 45) {
    bindCheck();
    GX_GLCALL(glDisableVertexAttribArray, index);
  } else {
    GX_GLCALL(glDisableVertexArrayAttrib, _vao, index);
  }
}

template<int VER>
void gx::GLVertexArray<VER>::setAttrib(
  GLuint index, GLBuffer<VER>& buffer, GLintptr offset, GLsizei stride,
  GLint size, GLenum type, GLboolean normalized)
{
  if constexpr (VER < 43) {
    bindCheck();
    bufferBindCheck(buffer);
    GX_GLCALL(glVertexAttribPointer, index, size, type, normalized, stride,
              reinterpret_cast<const void*>(offset));
  } else if constexpr (VER < 45) {
    // attribindex & bindingindex set the same for simplicity
    bindCheck();
    GX_GLCALL(glBindVertexBuffer, index, buffer.id(), offset, stride);
    GX_GLCALL(glVertexAttribBinding, index, index);
    GX_GLCALL(glVertexAttribFormat, index, size, type, normalized, 0);
  } else {
    // attribindex & bindingindex set the same for simplicity
    GX_GLCALL(glVertexArrayVertexBuffer, _vao, index, buffer.id(), offset, stride);
    GX_GLCALL(glVertexArrayAttribBinding, _vao, index, index);
    GX_GLCALL(glVertexArrayAttribFormat, _vao, index, size, type, normalized, 0);
  }
}

template<int VER>
void gx::GLVertexArray<VER>::setAttribI(
  GLuint index, GLBuffer<VER>& buffer, GLintptr offset,
  GLsizei stride, GLint size, GLenum type)
{
  if constexpr (VER < 43) {
    bindCheck();
    bufferBindCheck(buffer);
    GX_GLCALL(glVertexAttribIPointer, index, size, type, stride,
              reinterpret_cast<const void*>(offset));
  } else if constexpr (VER < 45) {
    // attribindex & bindingindex set the same for simplicity
    bindCheck();
    GX_GLCALL(glBindVertexBuffer, index, buffer.id(), offset, stride);
    GX_GLCALL(glVertexAttribBinding, index, index);
    GX_GLCALL(glVertexAttribIFormat, index, size, type, 0);
  } else {
    // attribindex & bindingindex set the same for simplicity
    GX_GLCALL(glVertexArrayVertexBuffer, _vao, index, buffer.id(), offset, stride);
    GX_GLCALL(glVertexArrayAttribBinding, _vao, index, index);
    GX_GLCALL(glVertexArrayAttribIFormat, _vao, index, size, type, 0);
  }
}

template<int VER>
void gx::GLVertexArray<VER>::setAttribL(
  GLuint index, GLBuffer<VER>& buffer, GLintptr offset,
  GLsizei stride, GLint size, GLenum type)
{
  if constexpr (VER < 43) {
    bindCheck();
    bufferBindCheck(buffer);
    GX_GLCALL(glVertexAttribLPointer, index, size, type, stride,
              reinterpret_cast<const void*>(offset));
  } else if constexpr (VER < 45) {
    // attribindex & bindingindex set the same for simplicity
    bindCheck();
    GX_GLCALL(glBindVertexBuffer, index, buffer.id(), offset, stride);
    GX_GLCALL(glVertexAttribBinding, index, index);
    GX_GLCALL(glVertexAttribLFormat, index, size, type, 0);
  } else {
    // attribindex & bindingindex set the same for simplicity
    GX_GLCALL(glVertexArrayVertexBuffer, _vao, index, buffer.id(), offset, stride);
    GX_GLCALL(glVertexArrayAttribBinding, _vao, index, index);
    GX_GLCALL(glVertexArrayAttribLFormat, _vao, index, size, type, 0);
  }
}

template<int VER>
void gx::GLVertexArray<VER>::setAttribDivisor(GLuint index, GLuint divisor)
{
  if constexpr (VER < 45) {
    bindCheck();
    GX_GLCALL(glVertexAttribDivisor, index, divisor);
  } else {
    GX_GLCALL(glVertexArrayBindingDivisor, _vao, index, divisor);
  }
}

template<int VER>
void gx::GLVertexArray<VER>::cleanup() noexcept
{
  if (_vao) {
    if constexpr (VER < 45) {
      if (GLLastVertexArrayBind == _vao) { GLLastVertexArrayBind = 0; }
    }
    GX_GLCALL(glDeleteVertexArrays, 1, &_vao);
  }
}
