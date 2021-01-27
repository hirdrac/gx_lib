//
// GLBuffer.hh
// Copyright (C) 2021 Richard Bradley
//
// wrapper for OpenGL buffer object
//

#pragma once
#include "OpenGL.hh"
#include <utility>


inline namespace GLNAMESPACE {

class GLBuffer
{
 public:
  GLBuffer() = default;
  inline GLBuffer(GLBuffer&& b) noexcept;
  ~GLBuffer() { if (GLInitialized) cleanup(); }

  // operators
  inline GLBuffer& operator=(GLBuffer&& b) noexcept;
  explicit operator bool() const { return _buffer; }

  // accessors
  [[nodiscard]] GLuint id() const { return _buffer; }
  [[nodiscard]] GLsizei size() const { return _size; }

  // methods
  inline GLuint init();
    // creates buffer object - only call after GL context creation
    // use setData() below to define size/contents

  inline GLuint init(GLsizei size, const GLvoid* data);
    // create immutable data store
    // allow data changes (only contents, not size) with setSubData()
    //   if data is null

  GLuint release() noexcept { return std::exchange(_buffer, 0); }
    // releases ownership of managed buffer object, returns object id

  inline void bind(GLenum target);
  inline static void unbind(GLenum target);
    // target: GL_ARRAY_BUFFER, GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER,
    //   GL_DRAW_INDIRECT_BUFFER, GL_ELEMENT_ARRAY_BUFFER,
    //   GL_PIXEL_PACK_BUFFER, GL_PIXEL_UNPACK_BUFFER, GL_TEXTURE_BUFFER,
    //   GL_TRANSFORM_FEEDBACK_BUFFER, GL_UNIFORM_BUFFER
    // GL4+ target:
    //  GL_ATOMIC_COUNTER_BUFFER(4.2), GL_DISPATCH_INDIRECT_BUFFER(4.3),
    //  GL_SHADER_STORAGE_BUFFER(4.3), GL_QUERY_BUFFER(4.4)

  inline void bindBase(GLenum target, GLuint index);
  inline void bindRange(GLenum target, GLuint index,
			GLintptr offset, GLsizeiptr size);
    // target: GL_TRANSFORM_FEEDBACK_BUFFER, GL_UNIFORM_BUFFER,
    //   GL_ATOMIC_COUNTER_BUFFER(4.2), GL_SHADER_STORAGE_BUFFER(4.3)

  inline void setData(GLsizei size, const void* data, GLenum usage);
    // usage: GL_STREAM_DRAW, GL_STREAM_READ, GL_STREAM_COPY,
    //   GL_STATIC_DRAW, GL_STATIC_READ, GL_STATIC_COPY,
    //   GL_DYNAMIC_DRAW, GL_DYNAMIC_READ, GL_DYNAMIC_COPY

  inline void setSubData(GLintptr offset, GLsizei size, const void* data);

  [[nodiscard]] inline void* map(GLenum access);
    // access: GL_READ_ONLY, GL_WRITE_ONLY, GL_READ_WRITE
  inline bool unmap();

  [[nodiscard]] inline void* mapRange(
    GLintptr offset, GLsizeiptr length, GLbitfield access);
  inline void flushMappedRange(GLintptr offset, GLsizeiptr length);

  [[nodiscard]] inline GLint getParameteri(GLenum pname);
    // pname: GL_BUFFER_ACCESS, GL_BUFFER_ACCESS_FLAGS,
    //   GL_BUFFER_IMMUTABLE_STORAGE, GL_BUFFER_MAPPED, GL_BUFFER_SIZE,
    //   GL_BUFFER_STORAGE_FLAGS, GL_BUFFER_USAGE
  [[nodiscard]] inline GLint64 getParameteri64(GLenum pname);
    // pname: GL_BUFFER_MAP_LENGTH, GL_BUFFER_MAP_OFFSET


  // convenience methods/overloads
  template <class T>
  GLuint init(const T& x) {
    return init(sizeof(x[0])*std::size(x), std::data(x)); }

  template <class T>
  void setData(const T& x, GLenum usage) {
    setData(sizeof(x[0])*std::size(x), std::data(x), usage); }

  template <class T>
  void setSubData(GLintptr offset, const T& x) {
    setSubData(offset, sizeof(x[0])*std::size(x), std::data(x)); }

 private:
  GLuint _buffer = 0;
  GLsizei _size = 0;
#ifdef GX_GL33
  GLenum _target = GL_NONE; // last bound target

  void bindCheck() {
    if (GLLastBufferBind != _buffer) {
      bind((_target != GL_NONE) ? _target : GL_ARRAY_BUFFER);
    }
  }
#endif

  inline void cleanup() noexcept;

  // prevent copy/assignment
  GLBuffer(const GLBuffer&) = delete;
  GLBuffer& operator=(const GLBuffer&) = delete;
};


// **** Inline Implementations ****
GLBuffer::GLBuffer(GLBuffer&& b) noexcept
  : _buffer(b.release()), _size(b._size)
{
#ifdef GX_GL33
  _target = b._target;
#endif
}

GLBuffer& GLBuffer::operator=(GLBuffer&& b) noexcept
{
  if (this != &b) {
    cleanup();
    _buffer = b.release();
    _size = b._size;
#ifdef GX_GL33
    _target = b._target;
#endif
  }
  return *this;
}

GLuint GLBuffer::init()
{
  cleanup();
  _size = 0;
#ifdef GX_GL33
  _target = GL_NONE;
  GLCALL(glGenBuffers, 1, &_buffer);
#else
  GLCALL(glCreateBuffers, 1, &_buffer);
#endif
  return _buffer;
}

GLuint GLBuffer::init(GLsizei size, const GLvoid* data)
{
  // for immutable data store:
  //  glBufferStorage(_target, size, data, flags);      // OGL 4.4
  //  glNamedBufferStorage(_buffer, size, data, flags); // OGL 4.5
  // -- flags --
  //  GL_DYNAMIC_STORAGE_BIT
  //  GL_MAP_READ_BIT
  //  GL_MAP_WRITE_BIT
  //  GL_MAP_PERSISTENT_BIT
  //  GL_MAP_COHERENT_BIT
  //  GL_CLIENT_STORAGE_BIT

  cleanup();
  _size = size;
#ifdef GX_GL33
  // immutable buffer not available, just make normal buffer
  _target = GL_ARRAY_BUFFER;
  GLCALL(glGenBuffers, 1, &_buffer);
  bindCheck();
  GLCALL(glBufferData, _target, size, data, data ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW);
#else
  GLCALL(glCreateBuffers, 1, &_buffer);
  GLCALL(glNamedBufferStorage, _buffer, size, data, data ? 0 : GL_DYNAMIC_STORAGE_BIT);
#endif
  return _buffer;
}

void GLBuffer::bind(GLenum target)
{
  // -- GL45 NOTES --
  // glBindBuffer() most likely only used for ELEMENT_ARRAY, DRAW_INDIRECT
  // use glBindBufferBase()/glBindBufferRange() for
  //   UNIFORM_SHADER, SHADER_STORAGE, ATOMIC_COUNTER, TRANSFORM_FEEDBACK
  // use glVertexArrayVertexBuffer() for ARRAY_BUFFER
  // use glTextureBuffer()/glBindTextureUnit() for TEXTURE_BUFFER

  GLCALL(glBindBuffer, target, _buffer);
#ifdef GX_GL33
  if (target == GL_ARRAY_BUFFER) {
    GLLastArrayBufferBind = _buffer;
  } else if (_target == GL_ARRAY_BUFFER) {
    GLLastArrayBufferBind = 0;
  }

  _target = target;
  GLLastBufferBind = _buffer;
#endif
}

void GLBuffer::bindBase(GLenum target, GLuint index)
{
  GLCALL(glBindBufferBase, target, index, _buffer);
#ifdef GX_GL33
  _target = target;
#endif
}

void GLBuffer::bindRange(GLenum target, GLuint index,
			 GLintptr offset, GLsizeiptr size)
{
  GLCALL(glBindBufferRange, target, index, _buffer, offset, size);
#ifdef GX_GL33
  _target = target;
#endif
}

void GLBuffer::unbind(GLenum target)
{
  GLCALL(glBindBuffer, target, 0);
#ifdef GX_GL33
  GLLastBufferBind = 0;
  if (target == GL_ARRAY_BUFFER) {
    GLLastArrayBufferBind = 0;
  }
#endif
}

void GLBuffer::setData(GLsizei size, const void* data, GLenum usage)
{
  _size = size;
#ifdef GX_GL33
  bindCheck();
  GLCALL(glBufferData, _target, size, data, usage);
#else
  GLCALL(glNamedBufferData, _buffer, size, data, usage);
#endif
}

void GLBuffer::setSubData(GLintptr offset, GLsizei size, const void* data)
{
#ifdef GX_GL33
  bindCheck();
  GLCALL(glBufferSubData, _target, offset, size, data);
#else
  GLCALL(glNamedBufferSubData, _buffer, offset, size, data);
#endif
}

void* GLBuffer::map(GLenum access)
{
#ifdef GX_GL33
  bindCheck();
  void* result = glMapBuffer(_target, access);
  #if GX_DEBUG_GL
  if (!result) { GLCheckErrors("glMapBuffer"); }
  #endif
#else
  void* result = glMapNamedBuffer(_buffer, access);
  #if GX_DEBUG_GL
  if (!result) { GLCheckErrors("glMapNamedBuffer"); }
  #endif
#endif
  return result;
}

bool GLBuffer::unmap()
{
#ifdef GX_GL33
  bindCheck();
  bool result = glUnmapBuffer(_target);
  #if GX_DEBUG_GL
  GLCheckErrors("glUnmapBuffer");
  #endif
#else
  bool result = glUnmapNamedBuffer(_buffer);
  #if GX_DEBUG_GL
  GLCheckErrors("glUnmapNamedBuffer");
  #endif
#endif
  return result;
}

void* GLBuffer::mapRange(GLintptr offset, GLsizeiptr length, GLbitfield access)
{
#ifdef GX_GL33
  bindCheck();
  void* result = glMapBufferRange(_target, offset, length, access);
  #if GX_DEBUG_GL
  if (!result) { GLCheckErrors("glMapBufferRange"); }
  #endif
#else
  void* result = glMapNamedBufferRange(_buffer, offset, length, access);
  #if GX_DEBUG_GL
  if (!result) { GLCheckErrors("glMapNamedBufferRange"); }
  #endif
#endif
  return result;
}

void GLBuffer::flushMappedRange(GLintptr offset, GLsizeiptr length)
{
#ifdef GX_GL33
  bindCheck();
  GLCALL(glFlushMappedBufferRange, _target, offset, length);
#else
  GLCALL(glFlushMappedNamedBufferRange, _buffer, offset, length);
#endif
}

GLint GLBuffer::getParameteri(GLenum pname)
{
  GLint result;
#ifdef GX_GL33
  bindCheck();
  GLCALL(glGetBufferParameteriv, _target, pname, &result);
#else
  GLCALL(glGetNamedBufferParameteriv, _buffer, pname, &result);
#endif
  return result;
}

GLint64 GLBuffer::getParameteri64(GLenum pname)
{
  GLint64 result;
#ifdef GX_GL33
  bindCheck();
  GLCALL(glGetBufferParameteri64v, _target, pname, &result);
#else
  GLCALL(glGetNamedBufferParameteri64v, _buffer, pname, &result);
#endif
  return result;
}

void GLBuffer::cleanup() noexcept
{
  if (_buffer) {
#ifdef GX_GL33
    if (GLLastBufferBind == _buffer) { GLLastBufferBind = 0; }
    if (GLLastArrayBufferBind == _buffer) { GLLastArrayBufferBind = 0; }
#endif
    GLCALL(glDeleteBuffers, 1, &_buffer);
  }
}

} // end GLNAMESPACE
