//
// gx/OpenGL.cc
// Copyright (C) 2022 Richard Bradley
//

#include "OpenGL.hh"
#include "Logger.hh"
#include <cstdlib>
using namespace gx;


inline namespace GX_GLNAMESPACE {

// **** Globals ****
bool GLInitialized = false;
#if defined(GX_GL33) || defined(GX_GL42) || defined(GX_GL43)
GLuint GLLastBufferBind = 0;
GLuint GLLastArrayBufferBind = 0;
GLuint GLLastVertexArrayBind = 0;
GLuint GLLastTextureBind = 0;
#endif


// **** Callbacks ****
static void GLCleanUp()
{
  // flag is checked by GL class destructors to prevent the calling of OpenGL
  // functions at process shutdown when a context no long exists
  GLInitialized = false;
}

#if !defined(GX_GL33) && !defined(GX_GL42)
static constexpr const char* GLSourceStr(GLenum source)
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

static constexpr const char* GLTypeStr(GLenum type)
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

static constexpr const char* GLSeverityStr(GLenum severity)
{
  switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:   return " severity=HIGH";
    case GL_DEBUG_SEVERITY_MEDIUM: return " severity=MEDIUM";
    case GL_DEBUG_SEVERITY_LOW:    return " severity=LOW";
    default:                       return "";
  }
}

static constexpr LogLevel GLSeverityLogLevel(GLenum severity)
{
  switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:
    case GL_DEBUG_SEVERITY_MEDIUM: return LVL_ERROR;
    case GL_DEBUG_SEVERITY_LOW:    return LVL_WARN;
    default:                       return LVL_INFO;
  }
}

static void APIENTRY GLDebugCB(
  GLenum source, GLenum type, GLuint id, GLenum severity,
  GLsizei length, const GLchar* message, const void* userParam)
{
  GX_LOGGER_LOG(
    defaultLogger(), GLSeverityLogLevel(severity),
    "GLDebug: source=", GLSourceStr(source), " type=", GLTypeStr(type),
    " id=", id, GLSeverityStr(severity), " message=[", message, ']');
}
#endif


// **** Functions ****
bool GLSetupContext(GLADloadproc loadProc)
{
  if (!GLInitialized) {
    if (gladLoadGLLoader(loadProc) == 0) {
      GX_LOG_ERROR("failed to setup GL context");
      return false;
    }

    std::atexit(GLCleanUp);
    GLInitialized = true;
  }

#if !defined(GX_GL33) && !defined(GX_GL42)
  // debug output available with GL4.3 or later
  GLint flags = 0;
  glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
  if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
    GX_LOG_INFO("OpenGL debug context enabled");
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(GLDebugCB, nullptr);
    glDebugMessageControl(
      GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
  }
#endif
  return true;
}

void GLClearState()
{
  // clear GL state
  glUseProgram(0);
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, 0);

#if defined(GX_GL33) || defined(GX_GL42) || defined(GX_GL43)
  GLLastBufferBind = 0;
  GLLastArrayBufferBind = 0;
  GLLastVertexArrayBind = 0;
  GLLastTextureBind = 0;
#endif
}

std::string GLErrorStr(GLenum error)
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

int GLCheckErrors(std::string_view msg, const char* file, int line)
{
  int count = 0;
  GLenum error;
  while ((error = glGetError()) != GL_NO_ERROR) {
    GX_LOGGER_LOG_FL(defaultLogger(), LVL_ERROR, file, line, msg, ": ",
                     GLErrorStr(error));
    ++count;
  }

  return count;
}

} // end GX_GLNAMESPACE
