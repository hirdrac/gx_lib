//
// gx/OpenGL.cc
// Copyright (C) 2026 Richard Bradley
//

#include "OpenGL.hh"
#include "Logger.hh"
#include <cstdlib>
using namespace gx;


// **** Callbacks ****
static void GLCleanUp()
{
  // flag is checked by GL class destructors to prevent the calling of OpenGL
  // functions at process shutdown when a context no long exists
  GLVersion = 0;
}

[[nodiscard]] static constexpr const char* GLSourceStr(GLenum source)
{
  const char* sourceStr = "unknown";
#define DEBUG_SOURCE_CASE(x) case GL_DEBUG_SOURCE_##x: sourceStr = #x; break
  switch (source) {
    DEBUG_SOURCE_CASE(API);
    DEBUG_SOURCE_CASE(WINDOW_SYSTEM);
    DEBUG_SOURCE_CASE(SHADER_COMPILER);
    DEBUG_SOURCE_CASE(THIRD_PARTY);
    DEBUG_SOURCE_CASE(APPLICATION);
    DEBUG_SOURCE_CASE(OTHER);
  }
#undef DEBUG_SOURCE_CASE
  return sourceStr;
}

[[nodiscard]] static constexpr const char* GLTypeStr(GLenum type)
{
  const char* typeStr = "unknown";
#define DEBUG_TYPE_CASE(x) case GL_DEBUG_TYPE_##x: typeStr = #x; break
  switch (type) {
    DEBUG_TYPE_CASE(ERROR);
    DEBUG_TYPE_CASE(DEPRECATED_BEHAVIOR);
    DEBUG_TYPE_CASE(UNDEFINED_BEHAVIOR);
    DEBUG_TYPE_CASE(PORTABILITY);
    DEBUG_TYPE_CASE(PERFORMANCE);
    DEBUG_TYPE_CASE(OTHER);
  }
#undef DEBUG_TYPE_CASE
  return typeStr;
}

[[nodiscard]] static constexpr const char* GLSeverityStr(GLenum severity)
{
  switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:   return " severity=HIGH";
    case GL_DEBUG_SEVERITY_MEDIUM: return " severity=MEDIUM";
    case GL_DEBUG_SEVERITY_LOW:    return " severity=LOW";
    default:                       return "";
  }
}

[[nodiscard]] static constexpr LogLevel GLSeverityLogLevel(GLenum severity)
{
  switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:
    case GL_DEBUG_SEVERITY_MEDIUM: return LogLevel::error;
    case GL_DEBUG_SEVERITY_LOW:    return LogLevel::warn;
    default:                       return LogLevel::info;
  }
}

static void GLDebugCB(
  GLenum source, GLenum type, GLuint id, GLenum severity,
  GLsizei length, const GLchar* message, const void* userParam)
{
  GX_LOGGER_LOG(
    defaultLogger(), GLSeverityLogLevel(severity),
    "GLDebug: source=", GLSourceStr(source), " type=", GLTypeStr(type),
    " id=", id, GLSeverityStr(severity), " message=[", message, ']');
}


// **** Functions ****
bool gx::GLSetupContext(GLADloadfunc loadProc)
{
  if (GLVersion == 0) {
    GLVersion = gladLoadGL(loadProc);
    if (GLVersion == 0) {
      GX_LOG_ERROR("failed to setup GL context");
      return false;
    }

    std::atexit(GLCleanUp);
  }

  GLint flags = 0;
  glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
  if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
    // debug output available with GL4.3 or later
    GX_LOG_INFO("OpenGL debug context enabled");
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(GLDebugCB, nullptr);
    glDebugMessageControl(
      GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
  }

  return true;
}

void gx::GLClearState()
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

std::string gx::GLErrorStr(GLenum error)
{
  switch (error) {
#define ERROR_CASE(x) case GL_##x: return #x
    ERROR_CASE(NO_ERROR);
    ERROR_CASE(INVALID_ENUM);
    ERROR_CASE(INVALID_VALUE);
    ERROR_CASE(INVALID_OPERATION);
    ERROR_CASE(INVALID_FRAMEBUFFER_OPERATION);
    ERROR_CASE(OUT_OF_MEMORY);
    ERROR_CASE(STACK_UNDERFLOW);
    ERROR_CASE(STACK_OVERFLOW);
#undef ERROR_CASE
    default: return std::to_string(error);
  }
}

int gx::GLCheckErrors(std::string_view msg, std::string_view file, int line)
{
  int count = 0;
  GLenum error;
  while ((error = glGetError()) != GL_NO_ERROR) {
    GX_LOGGER_LOG_FL(defaultLogger(), LogLevel::error, file, line,
                     msg, ": ", GLErrorStr(error));
    ++count;
  }

  return count;
}

void gx::GLSetUnpackAlignment(GLsizei width, GLenum format, GLenum type)
{
  const int x = width * GLPixelSize(format, type);
  int align = 8;
  if (x & 1) { align = 1; }
  else if (x & 2) { align = 2; }
  else if (x & 4) { align = 4; }

  GX_GLCALL(glPixelStorei, GL_UNPACK_ALIGNMENT, align);
}
