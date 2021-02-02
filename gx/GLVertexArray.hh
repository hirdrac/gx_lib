//
// gx/GLVertexArray.hh
// Copyright (C) 2021 Richard Bradley
//
// wrapper for OpenGL vertex array object
//

#pragma once
#include "GLBuffer.hh"
#include "OpenGL.hh"
#include <utility>


inline namespace GLNAMESPACE {

class GLVertexArray
{
 public:
  GLVertexArray() = default;
  inline GLVertexArray(GLVertexArray&& v) noexcept;
  ~GLVertexArray() { if (GLInitialized) cleanup(); }

  // operators
  inline GLVertexArray& operator=(GLVertexArray&& v) noexcept;
  explicit operator bool() const { return _vao; }

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
    GLuint index, GLBuffer& buffer, GLintptr offset,
    GLsizei stride, GLint size, GLenum type, GLboolean normalized);
  inline void setAttribI(
    GLuint index, GLBuffer& buffer, GLintptr offset,
    GLsizei stride, GLint size, GLenum type);
  inline void setAttribL(
    GLuint index, GLBuffer& buffer, GLintptr offset,
    GLsizei stride, GLint size, GLenum type);
  inline void setAttribDivisor(GLuint index, GLuint divisor);

 private:
  GLuint _vao = 0;

#ifdef GX_GL33
  void bindCheck() {
    if (GLLastVertexArrayBind != _vao) { bind(); }
  }

  void bufferBindCheck(GLBuffer& buffer) {
    if (GLLastArrayBufferBind != buffer.id()) {
      buffer.bind(GL_ARRAY_BUFFER);
    }
  }
#endif

  inline void cleanup() noexcept;

  // prevent copy/assignment
  GLVertexArray(const GLVertexArray&) = delete;
  GLVertexArray& operator=(const GLVertexArray&) = delete;
};


// **** Inline Implementation ****
GLVertexArray::GLVertexArray(GLVertexArray&& v) noexcept
  : _vao(v.release()) { }

GLVertexArray& GLVertexArray::operator=(GLVertexArray&& v) noexcept
{
  if (this != &v) {
    cleanup();
    _vao = v.release();
  }
  return *this;
}

GLuint GLVertexArray::init()
{
  cleanup();
#ifdef GX_GL33
  GLCALL(glGenVertexArrays, 1, &_vao);
  // VA created when bound for the first time
#else
  GLCALL(glCreateVertexArrays, 1, &_vao);
#endif
  return _vao;
}

void GLVertexArray::bind()
{
  GLCALL(glBindVertexArray, _vao);
#ifdef GX_GL33
  GLLastVertexArrayBind = _vao;
#endif
}

void GLVertexArray::unbind()
{
  GLCALL(glBindVertexArray, 0);
#ifdef GX_GL33
  GLLastVertexArrayBind = 0;
#endif
}

void GLVertexArray::enableAttrib(GLuint index)
{
#ifdef GX_GL33
  bindCheck();
  GLCALL(glEnableVertexAttribArray, index);
#else
  GLCALL(glEnableVertexArrayAttrib, _vao, index);
#endif
}

void GLVertexArray::disableAttrib(GLuint index)
{
#ifdef GX_GL33
  bindCheck();
  GLCALL(glDisableVertexAttribArray, index);
#else
  GLCALL(glDisableVertexArrayAttrib, _vao, index);
#endif
}

void GLVertexArray::setAttrib(
  GLuint index, GLBuffer& buffer, GLintptr offset, GLsizei stride,
  GLint size, GLenum type, GLboolean normalized)
{
#ifdef GX_GL33
  bindCheck();
  bufferBindCheck(buffer);
  GLCALL(glVertexAttribPointer, index, size, type, normalized, stride,
	 reinterpret_cast<const void*>(offset));
#else
  // attribindex & bindingindex set the same for simplicity
  GLCALL(glVertexArrayVertexBuffer, _vao, index, buffer.id(), offset, stride);
  GLCALL(glVertexArrayAttribBinding, _vao, index, index);
  GLCALL(glVertexArrayAttribFormat, _vao, index, size, type, normalized, 0);
#endif
}

void GLVertexArray::setAttribI(
  GLuint index, GLBuffer& buffer, GLintptr offset,
  GLsizei stride, GLint size, GLenum type)
{
#ifdef GX_GL33
  bindCheck();
  bufferBindCheck(buffer);
  GLCALL(glVertexAttribIPointer, index, size, type, stride,
	 reinterpret_cast<const void*>(offset));
#else
  // attribindex & bindingindex set the same for simplicity
  GLCALL(glVertexArrayVertexBuffer, _vao, index, buffer.id(), offset, stride);
  GLCALL(glVertexArrayAttribBinding, _vao, index, index);
  GLCALL(glVertexArrayAttribIFormat, _vao, index, size, type, 0);
#endif
}

void GLVertexArray::setAttribL(
  GLuint index, GLBuffer& buffer, GLintptr offset,
  GLsizei stride, GLint size, GLenum type)
{
#ifdef GX_GL33
  bindCheck();
  bufferBindCheck(buffer);
  GLCALL(glVertexAttribLPointer, index, size, type, stride,
	 reinterpret_cast<const void*>(offset));
#else
  // attribindex & bindingindex set the same for simplicity
  GLCALL(glVertexArrayVertexBuffer, _vao, index, buffer.id(), offset, stride);
  GLCALL(glVertexArrayAttribBinding, _vao, index, index);
  GLCALL(glVertexArrayAttribLFormat, _vao, index, size, type, 0);
#endif
}

void GLVertexArray::setAttribDivisor(GLuint index, GLuint divisor)
{
#ifdef GX_GL33
  bindCheck();
  GLCALL(glVertexAttribDivisor, index, divisor);
#else
  GLCALL(glVertexArrayBindingDivisor, _vao, index, divisor);
#endif
}

void GLVertexArray::cleanup() noexcept
{
  if (_vao) {
#ifdef GX_GL33
    if (GLLastVertexArrayBind == _vao) { GLLastVertexArrayBind = 0; }
#endif
    GLCALL(glDeleteVertexArrays, 1, &_vao);
  }
}

} // end GLNAMESPACE
