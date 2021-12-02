//
// gx/OpenGL.cc
// Copyright (C) 2021 Richard Bradley
//

#include "OpenGL.hh"
#include "Logger.hh"
#include <cstdlib>
using namespace gx;


inline namespace GX_GLNAMESPACE {

// **** Globals ****
bool GLInitialized = false;
#ifdef GX_GL33
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

#ifndef GX_GL33
static void APIENTRY GLDebugCB(
  GLenum source, GLenum type, GLuint id, GLenum severity,
  GLsizei length, const GLchar *message, const void *userParam)
{
  #define DEBUG_SOURCE_CASE(x) case GL_DEBUG_SOURCE_##x: sourceStr = #x; break
  const char* sourceStr = "unknown";
  switch (source) {
    DEBUG_SOURCE_CASE(API);
    DEBUG_SOURCE_CASE(WINDOW_SYSTEM);
    DEBUG_SOURCE_CASE(SHADER_COMPILER);
    DEBUG_SOURCE_CASE(THIRD_PARTY);
    DEBUG_SOURCE_CASE(APPLICATION);
    DEBUG_SOURCE_CASE(OTHER);
  }
  #undef DEBUG_SOURCE_CASE

  #define DEBUG_TYPE_CASE(x) case GL_DEBUG_TYPE_##x: typeStr = #x; break
  const char* typeStr = "unknown";
  switch (type) {
    DEBUG_TYPE_CASE(ERROR);
    DEBUG_TYPE_CASE(DEPRECATED_BEHAVIOR);
    DEBUG_TYPE_CASE(UNDEFINED_BEHAVIOR);
    DEBUG_TYPE_CASE(PORTABILITY);
    DEBUG_TYPE_CASE(PERFORMANCE);
    DEBUG_TYPE_CASE(OTHER);
  }
  #undef DEBUG_TYPE_CASE

  LogLevel lvl = LVL_ERROR;
  const char* severityStr = "";
  switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:
      severityStr = " severity=HIGH"; break;
    case GL_DEBUG_SEVERITY_MEDIUM:
      severityStr = " severity=MEDIUM"; break;
    case GL_DEBUG_SEVERITY_LOW:
      severityStr = " severity=LOW"; lvl = LVL_WARN; break;
    default:
      lvl = LVL_INFO; break;
  }

  GX_LOGGER_LOG(
    defaultLogger(), lvl, "GLDebug: source=", sourceStr, " type=",
    typeStr, " id=", id, severityStr, " message=[", message, ']');
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

#ifndef GX_GL33
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

#ifdef GX_GL33
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
