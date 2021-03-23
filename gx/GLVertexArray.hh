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


inline namespace GX_GLNAMESPACE {

class GLVertexArray
{
 public:
  GLVertexArray() = default;
  inline GLVertexArray(GLVertexArray&& v) noexcept;
  ~GLVertexArray() { if (GLInitialized) cleanup(); }

  // operators
  inline GLVertexArray& operator=(GLVertexArray&& v) noexcept;
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
  GX_GLCALL(glGenVertexArrays, 1, &_vao);
  // VA created when bound for the first time
#else
  GX_GLCALL(glCreateVertexArrays, 1, &_vao);
#endif
  return _vao;
}

void GLVertexArray::bind()
{
  GX_GLCALL(glBindVertexArray, _vao);
#ifdef GX_GL33
  GLLastVertexArrayBind = _vao;
#endif
}

void GLVertexArray::unbind()
{
  GX_GLCALL(glBindVertexArray, 0);
#ifdef GX_GL33
  GLLastVertexArrayBind = 0;
#endif
}

void GLVertexArray::enableAttrib(GLuint index)
{
#ifdef GX_GL33
  bindCheck();
  GX_GLCALL(glEnableVertexAttribArray, index);
#else
  GX_GLCALL(glEnableVertexArrayAttrib, _vao, index);
#endif
}

void GLVertexArray::disableAttrib(GLuint index)
{
#ifdef GX_GL33
  bindCheck();
  GX_GLCALL(glDisableVertexAttribArray, index);
#else
  GX_GLCALL(glDisableVertexArrayAttrib, _vao, index);
#endif
}

void GLVertexArray::setAttrib(
  GLuint index, GLBuffer& buffer, GLintptr offset, GLsizei stride,
  GLint size, GLenum type, GLboolean normalized)
{
#ifdef GX_GL33
  bindCheck();
  bufferBindCheck(buffer);
  GX_GLCALL(glVertexAttribPointer, index, size, type, normalized, stride,
            reinterpret_cast<const void*>(offset));
#else
  // attribindex & bindingindex set the same for simplicity
  GX_GLCALL(glVertexArrayVertexBuffer, _vao, index, buffer.id(), offset, stride);
  GX_GLCALL(glVertexArrayAttribBinding, _vao, index, index);
  GX_GLCALL(glVertexArrayAttribFormat, _vao, index, size, type, normalized, 0);
#endif
}

void GLVertexArray::setAttribI(
  GLuint index, GLBuffer& buffer, GLintptr offset,
  GLsizei stride, GLint size, GLenum type)
{
#ifdef GX_GL33
  bindCheck();
  bufferBindCheck(buffer);
  GX_GLCALL(glVertexAttribIPointer, index, size, type, stride,
	 reinterpret_cast<const void*>(offset));
#else
  // attribindex & bindingindex set the same for simplicity
  GX_GLCALL(glVertexArrayVertexBuffer, _vao, index, buffer.id(), offset, stride);
  GX_GLCALL(glVertexArrayAttribBinding, _vao, index, index);
  GX_GLCALL(glVertexArrayAttribIFormat, _vao, index, size, type, 0);
#endif
}

void GLVertexArray::setAttribL(
  GLuint index, GLBuffer& buffer, GLintptr offset,
  GLsizei stride, GLint size, GLenum type)
{
#ifdef GX_GL33
  bindCheck();
  bufferBindCheck(buffer);
  GX_GLCALL(glVertexAttribLPointer, index, size, type, stride,
            reinterpret_cast<const void*>(offset));
#else
  // attribindex & bindingindex set the same for simplicity
  GX_GLCALL(glVertexArrayVertexBuffer, _vao, index, buffer.id(), offset, stride);
  GX_GLCALL(glVertexArrayAttribBinding, _vao, index, index);
  GX_GLCALL(glVertexArrayAttribLFormat, _vao, index, size, type, 0);
#endif
}

void GLVertexArray::setAttribDivisor(GLuint index, GLuint divisor)
{
#ifdef GX_GL33
  bindCheck();
  GX_GLCALL(glVertexAttribDivisor, index, divisor);
#else
  GX_GLCALL(glVertexArrayBindingDivisor, _vao, index, divisor);
#endif
}

void GLVertexArray::cleanup() noexcept
{
  if (_vao) {
#ifdef GX_GL33
    if (GLLastVertexArrayBind == _vao) { GLLastVertexArrayBind = 0; }
#endif
    GX_GLCALL(glDeleteVertexArrays, 1, &_vao);
  }
}

} // end GX_GLNAMESPACE
