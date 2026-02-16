//
// gx/OpenGL.cc
// Copyright (C) 2026 Richard Bradley
//

#include "OpenGL.hh"
#include "Logger.hh"
#include <cstdlib>
using namespace gx;


// **** Callbacks ****
static void cleanUp()
{
  // flag is checked by GL class destructors to prevent the calling of OpenGL
  // functions at process shutdown when a context no long exists
  GLVersion = 0;
}

[[nodiscard]] static constexpr const char* getGLSourceStr(GLenum source)
{
  const char* str = "unknown";
#define GX_CASE(x) case GL_DEBUG_SOURCE_##x: str = #x; break
  switch (source) {
    GX_CASE(API);
    GX_CASE(WINDOW_SYSTEM);
    GX_CASE(SHADER_COMPILER);
    GX_CASE(THIRD_PARTY);
    GX_CASE(APPLICATION);
    GX_CASE(OTHER);
  }
#undef GX_CASE
  return str;
}

[[nodiscard]] static constexpr const char* getGLTypeStr(GLenum type)
{
  const char* str = "unknown";
#define GX_CASE(x) case GL_DEBUG_TYPE_##x: str = #x; break
  switch (type) {
    GX_CASE(ERROR);
    GX_CASE(DEPRECATED_BEHAVIOR);
    GX_CASE(UNDEFINED_BEHAVIOR);
    GX_CASE(PORTABILITY);
    GX_CASE(PERFORMANCE);
    GX_CASE(OTHER);
  }
#undef GX_CASE
  return str;
}

[[nodiscard]] static constexpr const char* getGLSeverityStr(GLenum severity)
{
  switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:   return " severity=HIGH";
    case GL_DEBUG_SEVERITY_MEDIUM: return " severity=MEDIUM";
    case GL_DEBUG_SEVERITY_LOW:    return " severity=LOW";
    default:                       return "";
  }
}

[[nodiscard]] static constexpr LogLevel getGLSeverityLogLevel(GLenum severity)
{
  switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:
    case GL_DEBUG_SEVERITY_MEDIUM: return LogLevel::error;
    case GL_DEBUG_SEVERITY_LOW:    return LogLevel::warn;
    default:                       return LogLevel::info;
  }
}

static void debugCB(
  GLenum source, GLenum type, GLuint id, GLenum severity,
  GLsizei length, const GLchar* message, const void* userParam)
{
  GX_LOGGER_LOG(
    defaultLogger(), getGLSeverityLogLevel(severity),
    "GLDebug: source=", getGLSourceStr(source), " type=", getGLTypeStr(type),
    " id=", id, getGLSeverityStr(severity), " message=[", message, ']');
}


// **** Functions ****
bool gx::setupGLContext(GLADloadfunc loadProc)
{
  if (GLVersion == 0) {
    GLVersion = gladLoadGL(loadProc);
    if (GLVersion == 0) {
      GX_LOG_ERROR("failed to setup GL context");
      return false;
    }

    std::atexit(cleanUp);
  }

  GLint flags = 0;
  glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
  if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
    // debug output available with GL4.3 or later
    GX_LOG_INFO("OpenGL debug context enabled");
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(debugCB, nullptr);
    glDebugMessageControl(
      GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
  }

  return true;
}

void gx::clearGLState()
{
  // clear GL state
  glUseProgram(0);
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  GLLastArrayBufferBind = 0;
  GLLastCopyWriteBufferBind = 0;
  GLLastVertexArrayBind = 0;
  GLLastTextureBind = 0;
  GLLastFramebufferBind = 0;
}

std::string gx::getGLErrorStr(GLenum error)
{
  switch (error) {
#define GX_CASE(x) case GL_##x: return #x
    GX_CASE(NO_ERROR);
    GX_CASE(INVALID_ENUM);
    GX_CASE(INVALID_VALUE);
    GX_CASE(INVALID_OPERATION);
    GX_CASE(INVALID_FRAMEBUFFER_OPERATION);
    GX_CASE(OUT_OF_MEMORY);
    GX_CASE(STACK_UNDERFLOW);
    GX_CASE(STACK_OVERFLOW);
#undef GX_CASE
    default: return std::to_string(error);
  }
}

int gx::checkGLErrors(std::string_view msg, std::string_view file, int line)
{
  int count = 0;
  GLenum error;
  while ((error = glGetError()) != GL_NO_ERROR) {
    GX_LOGGER_LOG_FL(defaultLogger(), LogLevel::error, file, line,
                     msg, ": ", getGLErrorStr(error));
    ++count;
  }

  return count;
}

void gx::setGLUnpackAlignment(GLsizei width, GLenum format, GLenum type)
{
  const int x = width * getGLPixelSize(format, type);
  int align = 8;
  if (x & 1) { align = 1; }
  else if (x & 2) { align = 2; }
  else if (x & 4) { align = 4; }

  GX_GLCALL(glPixelStorei, GL_UNPACK_ALIGNMENT, align);
}
