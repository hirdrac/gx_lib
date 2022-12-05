//
// gx/GLBuffer.hh
// Copyright (C) 2022 Richard Bradley
//
// wrapper for OpenGL buffer object
//

#pragma once
#include "OpenGL.hh"
#include <utility>

namespace gx {
  template<int VER> class GLBuffer;
}

template<int VER>
class gx::GLBuffer
{
 public:
  using type = GLBuffer<VER>;

  GLBuffer() = default;
  ~GLBuffer() { if (GLInitialized) cleanup(); }

  // prevent copy/assignment
  GLBuffer(const type&) = delete;
  type& operator=(const type&) = delete;

  // enable move
  inline GLBuffer(type&& b) noexcept;
  inline type& operator=(type&& b) noexcept;

  // operators
  [[nodiscard]] explicit operator bool() const { return _buffer; }

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
  template<class T>
  GLuint init(const T& x) {
    return init(sizeof(x[0])*std::size(x), std::data(x)); }

  template<class T>
  void setData(const T& x, GLenum usage) {
    setData(GLsizei(sizeof(x[0])*std::size(x)), std::data(x), usage); }

  template<class T>
  void setSubData(GLintptr offset, const T& x) {
    setSubData(offset, GLsizei(sizeof(x[0])*std::size(x)), std::data(x)); }

 private:
  GLuint _buffer = 0;
  GLsizei _size = 0;

  void bindCheck(GLenum target) {
    if ((target == GL_ARRAY_BUFFER && GLLastArrayBufferBind == _buffer)
        || (target == GL_COPY_WRITE_BUFFER && GLLastCopyWriteBufferBind == _buffer)) { return; }
    bind(target);
  }

  void clearBind() {
    if (GLLastArrayBufferBind == _buffer) GLLastArrayBufferBind = 0;
    if (GLLastCopyWriteBufferBind == _buffer) GLLastCopyWriteBufferBind = 0;
  }

  inline void cleanup() noexcept;
};


// **** Inline Implementations ****
template<int VER>
gx::GLBuffer<VER>::GLBuffer(GLBuffer<VER>&& b) noexcept
  : _buffer{b.release()}, _size{b._size}
{ }

template<int VER>
gx::GLBuffer<VER>& gx::GLBuffer<VER>::operator=(GLBuffer<VER>&& b) noexcept
{
  if (this != &b) {
    cleanup();
    _buffer = b.release();
    _size = b._size;
  }
  return *this;
}

template<int VER>
GLuint gx::GLBuffer<VER>::init()
{
  cleanup();
  _size = 0;
  if constexpr (VER < 45) {
    GX_GLCALL(glGenBuffers, 1, &_buffer);
  } else {
    GX_GLCALL(glCreateBuffers, 1, &_buffer);
  }
  return _buffer;
}

template<int VER>
GLuint gx::GLBuffer<VER>::init(GLsizei size, const GLvoid* data)
{
  // for immutable data store:
  //  glBufferStorage(target, size, data, flags);       // OGL 4.4
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
  if constexpr (VER < 45) {
    // immutable buffer not available, just make normal buffer
    GX_GLCALL(glGenBuffers, 1, &_buffer);
    bindCheck(GL_COPY_WRITE_BUFFER);
    GX_GLCALL(glBufferData, GL_COPY_WRITE_BUFFER, size, data, data ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW);
  } else {
    GX_GLCALL(glCreateBuffers, 1, &_buffer);
    GX_GLCALL(glNamedBufferStorage, _buffer, size, data, data ? 0 : GL_DYNAMIC_STORAGE_BIT);
  }
  return _buffer;
}

template<int VER>
void gx::GLBuffer<VER>::bind(GLenum target)
{
  // -- GL45 NOTES --
  // glBindBuffer() most likely only used for ELEMENT_ARRAY, DRAW_INDIRECT
  // use glBindBufferBase()/glBindBufferRange() for
  //   UNIFORM_SHADER, SHADER_STORAGE, ATOMIC_COUNTER, TRANSFORM_FEEDBACK
  // use glVertexArrayVertexBuffer() for ARRAY_BUFFER
  // use glTextureBuffer()/glBindTextureUnit() for TEXTURE_BUFFER

  GX_GLCALL(glBindBuffer, target, _buffer);
  if constexpr (VER < 45) {
    clearBind();
    if (target == GL_ARRAY_BUFFER) {
      GLLastArrayBufferBind = _buffer;
    } else if (target == GL_COPY_WRITE_BUFFER) {
      GLLastCopyWriteBufferBind = _buffer;
    }
  }
}

template<int VER>
void gx::GLBuffer<VER>::bindBase(GLenum target, GLuint index)
{
  // buffer target must be one of these:
  //   GL_ATOMIC_COUNTER_BUFFER, GL_TRANSFORM_FEEDBACK_BUFFER,
  //   GL_UNIFORM_BUFFER, GL_SHADER_STORAGE_BUFFER

  GX_GLCALL(glBindBufferBase, target, index, _buffer);
  if constexpr (VER < 45) { clearBind(); }
}

template<int VER>
void gx::GLBuffer<VER>::bindRange(
  GLenum target, GLuint index, GLintptr offset, GLsizeiptr size)
{
  // buffer target must be one of these:
  //   GL_ATOMIC_COUNTER_BUFFER, GL_TRANSFORM_FEEDBACK_BUFFER,
  //   GL_UNIFORM_BUFFER, GL_SHADER_STORAGE_BUFFER

  GX_GLCALL(glBindBufferRange, target, index, _buffer, offset, size);
  if constexpr (VER < 45) { clearBind(); }
}

template<int VER>
void gx::GLBuffer<VER>::unbind(GLenum target)
{
  GX_GLCALL(glBindBuffer, target, 0);
  if constexpr (VER < 45) {
    if (target == GL_ARRAY_BUFFER) {
      GLLastArrayBufferBind = 0;
    } else if (target == GL_COPY_WRITE_BUFFER) {
      GLLastCopyWriteBufferBind = 0;
    }
  }
}

template<int VER>
void gx::GLBuffer<VER>::setData(GLsizei size, const void* data, GLenum usage)
{
  _size = size;
  if constexpr (VER < 45) {
    bindCheck(GL_COPY_WRITE_BUFFER);
    GX_GLCALL(glBufferData, GL_COPY_WRITE_BUFFER, size, data, usage);
  } else {
    GX_GLCALL(glNamedBufferData, _buffer, size, data, usage);
  }
}

template<int VER>
void gx::GLBuffer<VER>::setSubData(GLintptr offset, GLsizei size, const void* data)
{
  if constexpr (VER < 45) {
    bindCheck(GL_COPY_WRITE_BUFFER);
    GX_GLCALL(glBufferSubData, GL_COPY_WRITE_BUFFER, offset, size, data);
  } else {
    GX_GLCALL(glNamedBufferSubData, _buffer, offset, size, data);
  }
}

template<int VER>
void* gx::GLBuffer<VER>::map(GLenum access)
{
  void* result;
  if constexpr (VER < 45) {
    bindCheck(GL_COPY_WRITE_BUFFER);
    result = glMapBuffer(GL_COPY_WRITE_BUFFER, access);
#ifdef GX_DEBUG_GL
    if (!result) { GLCheckErrors("glMapBuffer"); }
#endif
  } else {
    result = glMapNamedBuffer(_buffer, access);
#ifdef GX_DEBUG_GL
    if (!result) { GLCheckErrors("glMapNamedBuffer"); }
#endif
  }
  return result;
}

template<int VER>
bool gx::GLBuffer<VER>::unmap()
{
  bool result;
  if constexpr (VER < 45) {
    bindCheck(GL_COPY_WRITE_BUFFER);
    result = glUnmapBuffer(GL_COPY_WRITE_BUFFER);
#ifdef GX_DEBUG_GL
    GLCheckErrors("glUnmapBuffer");
#endif
  } else {
    result = glUnmapNamedBuffer(_buffer);
#ifdef GX_DEBUG_GL
    GLCheckErrors("glUnmapNamedBuffer");
#endif
  }
  return result;
}

template<int VER>
void* gx::GLBuffer<VER>::mapRange(
  GLintptr offset, GLsizeiptr length, GLbitfield access)
{
  void* result;
  if constexpr (VER < 45) {
    bindCheck(GL_COPY_WRITE_BUFFER);
    result = glMapBufferRange(GL_COPY_WRITE_BUFFER, offset, length, access);
#ifdef GX_DEBUG_GL
    if (!result) { GLCheckErrors("glMapBufferRange"); }
#endif
  } else {
    result = glMapNamedBufferRange(_buffer, offset, length, access);
#ifdef GX_DEBUG_GL
    if (!result) { GLCheckErrors("glMapNamedBufferRange"); }
#endif
  }
  return result;
}

template<int VER>
void gx::GLBuffer<VER>::flushMappedRange(GLintptr offset, GLsizeiptr length)
{
  if constexpr (VER < 45) {
    bindCheck(GL_COPY_WRITE_BUFFER);
    GX_GLCALL(glFlushMappedBufferRange, GL_COPY_WRITE_BUFFER, offset, length);
  } else {
    GX_GLCALL(glFlushMappedNamedBufferRange, _buffer, offset, length);
  }
}

template<int VER>
GLint gx::GLBuffer<VER>::getParameteri(GLenum pname)
{
  GLint result;
  if constexpr (VER < 45) {
    bindCheck(GL_COPY_WRITE_BUFFER);
    GX_GLCALL(glGetBufferParameteriv, GL_COPY_WRITE_BUFFER, pname, &result);
  } else {
    GX_GLCALL(glGetNamedBufferParameteriv, _buffer, pname, &result);
  }
  return result;
}

template<int VER>
GLint64 gx::GLBuffer<VER>::getParameteri64(GLenum pname)
{
  GLint64 result;
  if constexpr (VER < 45) {
    bindCheck(GL_COPY_WRITE_BUFFER);
    GX_GLCALL(glGetBufferParameteri64v, GL_COPY_WRITE_BUFFER, pname, &result);
  } else {
    GX_GLCALL(glGetNamedBufferParameteri64v, _buffer, pname, &result);
  }
  return result;
}

template<int VER>
void gx::GLBuffer<VER>::cleanup() noexcept
{
  if (_buffer) {
    if constexpr (VER < 45) { clearBind(); }
    GX_GLCALL(glDeleteBuffers, 1, &_buffer);
  }
}
