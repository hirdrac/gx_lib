//
// gx/OpenGLRenderer.cc
// Copyright (C) 2020 Richard Bradley
//

// TODO - add blur transparency shader

#include "OpenGLRenderer.hh"
#include "DrawList.hh"
#include "Image.hh"
#include "Color.hh"
#include "Logger.hh"
#include "OpenGL.hh"
//#include "Print.hh"
#include <GLFW/glfw3.h>
#include <cstdlib>

namespace {
  // **** Helper Functions ****
  void setCurrentContext(GLFWwindow* win)
  {
    // TODO - add lastThreadID check in debug
    // - if window hasn't changed, it must always be the same thread
    // - if window changes and threadID changes, call
    //   glfwMakeContextCurrent(nullptr) first to flush last context
    static GLFWwindow* lastWin = nullptr;
    if (lastWin != win) {
      lastWin = win;
      glfwMakeContextCurrent(win);
    }
  }

  GLProgram makeProgram(const char* vsrc, const char* fsrc)
  {
    GLShader vshader;
    if (!vshader.init(GL_VERTEX_SHADER, vsrc, GLSL_SOURCE_HEADER)) {
      LOG_ERROR("vshader error: ", vshader.infoLog());
      return GLProgram();
    }

    GLShader fshader;
    if (!fshader.init(GL_FRAGMENT_SHADER, fsrc, GLSL_SOURCE_HEADER)) {
      LOG_ERROR("fshader error: ", fshader.infoLog());
      return GLProgram();
    }

    GLProgram p;
    if (!p.init(vshader, fshader)) {
      LOG_ERROR("program link error: ", p.infoLog());
      return GLProgram();
    }

    return p;
  }

  GLint getUniformLoc(const GLProgram& p, const char* name)
  {
    GLint loc = p.getUniformLocation(name);
    if (loc < 0) {
      LOG_ERROR("unknown uniform location '", name, "' for prog ", p.id());
    }
    return loc;
  }

  // DrawEntry iterator reading helper functions
  template <typename T>
  uint32_t uval(T& itr) { return (itr++)->uval; }

  template <typename T>
  float fval(T& itr) { return (itr++)->fval; }

  template <typename T>
  gx::Vec2 fval2(T& itr) { return {fval(itr), fval(itr)}; }

  // **** Callbacks ****
  void CleanUpOpenGL()
  {
    // flag is checked by GL class destructors
    GLInitialized = false;
  }
}

void gx::OpenGLRenderer::setWindowHints(bool debug)
{
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, GL_VERSION_MAJOR);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, GL_VERSION_MINOR);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, debug ? GLFW_TRUE : GLFW_FALSE);
}

bool gx::OpenGLRenderer::init(GLFWwindow* win)
{
  static bool init_done = false;
  if (!init_done) {
    init_done = true;
    std::atexit(CleanUpOpenGL);
  }

  _window = win;
  setCurrentContext(win);
  if (!GLSetupContext(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
    return false;
  }

  glfwSwapInterval(1); // enable V-SYNC

  // solid color shader
  static const char* SP0V_SRC =
    "layout(location = 0) in vec4 in_pos;" // x,y
    "layout(location = 1) in uint in_color;"
    "uniform mat4 trans;"
    "uniform vec4 modColor;"
    "out vec4 v_color;"
    "void main() {"
    "  v_color = unpackUnorm4x8(in_color) * modColor;"
    "  gl_Position = trans * vec4(in_pos.xy, 0, 1);"
    "}";
  static const char* SP0F_SRC =
    "in vec4 v_color;"
    "out vec4 fragColor;"
    "void main() { fragColor = v_color; }";
  _sp0 = makeProgram(SP0V_SRC, SP0F_SRC);
  _sp0_trans = getUniformLoc(_sp0, "trans");
  _sp0_modColor = getUniformLoc(_sp0, "modColor");

  // mono color texture shader (fonts)
  static const char* SP1V_SRC =
    "layout(location = 0) in vec4 in_pos;" // x,y,s,t
    "layout(location = 1) in uint in_color;"
    "uniform mat4 trans;"
    "uniform vec4 modColor;"
    "out vec4 v_color;"
    "out vec2 v_texCoord;"
    "void main() {"
    "  v_color = unpackUnorm4x8(in_color) * modColor;"
    "  v_texCoord = in_pos.zw;"
    "  gl_Position = trans * vec4(in_pos.xy, 0, 1);"
    "}";
  static const char* SP1F_SRC =
    "in vec2 v_texCoord;"
    "in vec4 v_color;"
    "uniform sampler2D texUnit;"
    "out vec4 fragColor;"
    "void main() {"
    "  float a = texture(texUnit, v_texCoord).r;"
    "  if (a == 0.0) discard;"
    "  fragColor = vec4(v_color.rgb, v_color.a * a);"
    "}";
  _sp1 = makeProgram(SP1V_SRC, SP1F_SRC);
  _sp1_trans = getUniformLoc(_sp1, "trans");
  _sp1_modColor = getUniformLoc(_sp1, "modColor");
  _sp1_texUnit = getUniformLoc(_sp1, "texUnit");

  // full color texture shader (images)
  static const char* SP2F_SRC =
    "in vec2 v_texCoord;"
    "in vec4 v_color;"
    "uniform sampler2D texUnit;"
    "out vec4 fragColor;"
    "void main() { fragColor = texture(texUnit, v_texCoord) * v_color; }";
  _sp2 = makeProgram(SP1V_SRC, SP2F_SRC);
  _sp2_trans = getUniformLoc(_sp2, "trans");
  _sp2_modColor = getUniformLoc(_sp2, "modColor");
  _sp2_texUnit = getUniformLoc(_sp2, "texUnit");

#if 0
  // debug output
  float val[2] = {};
  glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, val);
  println("GL_ALIASED_LINE_WIDTH_RANGE: ", val[0], " ", val[1]);

  glGetFloatv(GL_SMOOTH_LINE_WIDTH_RANGE, val);
  println("GL_SMOOTH_LINE_WIDTH_RANGE: ", val[0], " ", val[1]);

  glGetFloatv(GL_SMOOTH_LINE_WIDTH_GRANULARITY, val);
  println("GL_SMOOTH_LINE_WIDTH_GRANULARITY: ", val[0]);
#endif

  return _sp0 && _sp1 && _sp2;
}

gx::TextureID gx::OpenGLRenderer::setTexture(
  TextureID id, const Image& img, FilterType minFilter, FilterType magFilter)
{
  GLenum texformat, imgformat;
  switch (img.channels()) {
    case 1: texformat = GL_R8;    imgformat = GL_RED;  break;
    case 3: texformat = GL_RGB8;  imgformat = GL_RGB;  break;
    case 4: texformat = GL_RGBA8; imgformat = GL_RGBA; break;
    default: return 0;
  }

  TextureEntry* ePtr;
  if (id <= 0) {
    // new texture entry
    ePtr = &_textures[++_lastTexID];
    id = _lastTexID;
  } else {
    // update existing entry
    auto itr = _textures.find(id);
    if (itr == _textures.end()) { return 0; }
    ePtr = &(itr->second);
  }

  GLTexture2D& t = ePtr->tex;
  setCurrentContext(_window);
  if (!t || t.width() != img.width() || t.height() != img.height()
      || t.internalFormat() != texformat) {
    t.init(1, texformat, img.width(), img.height());
    ePtr->shader = (img.channels() == 1) ? 1 : 2;
  }

  t.setSubImage2D(
    0, 0, 0, img.width(), img.height(), imgformat, img.data());

  // TODO - make these configurable
  t.setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  t.setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // GL_CLAMP_TO_EDGE
    // GL_CLAMP_TO_BORDER
    // GL_MIRRORED_REPEAT
    // GL_REPEAT
    // GL_MIRROR_CLAMP_TO_EDGE

  if (minFilter != FILTER_UNSPECIFIED) {
    t.setParameter(GL_TEXTURE_MIN_FILTER,
                   (minFilter == FILTER_LINEAR) ? GL_LINEAR : GL_NEAREST);
    // other values:
    // GL_NEAREST_MIPMAP_NEAREST
    // GL_LINEAR_MIPMAP_NEAREST
    // GL_NEAREST_MIPMAP_LINEAR
    // GL_LINEAR_MIPMAP_LINEAR
  }

  if (magFilter != FILTER_UNSPECIFIED) {
    t.setParameter(GL_TEXTURE_MAG_FILTER,
                   (magFilter == FILTER_LINEAR) ? GL_LINEAR : GL_NEAREST);
  }

  return id;
}

void gx::OpenGLRenderer::clearFrame(int width, int height)
{
  _width = width;
  _height = height;
  _changed = true;
  _vertices.clear();
  _drawCalls.clear();
}

void gx::OpenGLRenderer::draw(const DrawList& dl, const Color& modColor)
{
  _changed = true;
  auto& entries = dl.entries();

  int size = 0; // vertices needed
  for (auto itr = entries.cbegin(), end = entries.cend(); itr != end; ) {
    DrawCmd cmd = itr->cmd;
    switch (cmd) {
      case CMD_color:      itr += 2; break;
      case CMD_lineWidth:  itr += 2; break;
      case CMD_texture:    itr += 2; break;
      case CMD_line:       itr += 5;  size += 2; break;
      case CMD_triangle:   itr += 7;  size += 3; break;
      case CMD_triangleT:  itr += 13; size += 3; break;
      case CMD_triangleC:  itr += 10; size += 3; break;
      case CMD_triangleTC: itr += 16; size += 3; break;
      case CMD_quad:       itr += 9;  size += 6; break;
      case CMD_quadT:      itr += 17; size += 6; break;
      case CMD_quadC:      itr += 13; size += 6; break;
      case CMD_quadTC:     itr += 21; size += 6; break;
      case CMD_rectangle:  itr += 5;  size += 6; break;
      case CMD_rectangleT: itr += 9;  size += 6; break;
      default:
        itr = end; // stop reading at first invalid cmd
        LOG_ERROR("unknown DrawCmd value: ", int(cmd));
	break;
    }
  }

  // general triangle layout
  //  0--1
  //  | /|
  //  |/ |
  //  2--3

  _vertices.reserve(_vertices.size() + size);
  uint32_t color = 0xffffffff;
  float lw = 1.0f;
  TextureID tid = 0;

  for (auto itr = entries.cbegin(), end = entries.cend(); itr != end; ) {
    DrawCmd cmd = (itr++)->cmd;
    switch (cmd) {
      case CMD_color:     color = uval(itr); break;
      case CMD_lineWidth: lw    = fval(itr); break;
      case CMD_texture:   tid   = uval(itr); break;

      case CMD_line: {
	addVertex(fval2(itr), color);
	addVertex(fval2(itr), color);
	addDrawCall(GL_LINES, 2, modColor, 0, lw);
	break;
      }
      case CMD_triangle: {
	addVertex(fval2(itr), color);
	addVertex(fval2(itr), color);
	addVertex(fval2(itr), color);
	addDrawCall(GL_TRIANGLES, 3, modColor, 0, lw);
	break;
      }
      case CMD_triangleT: {
        Vec2 p0 = fval2(itr), t0 = fval2(itr);
        Vec2 p1 = fval2(itr), t1 = fval2(itr);
        Vec2 p2 = fval2(itr), t2 = fval2(itr);
        addVertex(p0, t0, color);
	addVertex(p1, t1, color);
	addVertex(p2, t2, color);
	addDrawCall(GL_TRIANGLES, 3, modColor, tid, lw);
        break;
      }
      case CMD_triangleC: {
        Vec2 p0 = fval2(itr); uint32_t c0 = uval(itr);
        Vec2 p1 = fval2(itr); uint32_t c1 = uval(itr);
        Vec2 p2 = fval2(itr); uint32_t c2 = uval(itr);
        addVertex(p0, c0);
	addVertex(p1, c1);
	addVertex(p2, c2);
	addDrawCall(GL_TRIANGLES, 3, modColor, 0, lw);
        break;
      }
      case CMD_triangleTC: {
        Vec2 p0 = fval2(itr), t0 = fval2(itr); uint32_t c0 = uval(itr);
        Vec2 p1 = fval2(itr), t1 = fval2(itr); uint32_t c1 = uval(itr);
        Vec2 p2 = fval2(itr), t2 = fval2(itr); uint32_t c2 = uval(itr);
        addVertex(p0, t0, c0);
	addVertex(p1, t1, c1);
	addVertex(p2, t2, c2);
	addDrawCall(GL_TRIANGLES, 3, modColor, tid, lw);
        break;
      }
      case CMD_quad: {
        Vec2 p0 = fval2(itr), p1 = fval2(itr);
        Vec2 p2 = fval2(itr), p3 = fval2(itr);
	addVertex(p0, color);
	addVertex(p1, color);
	addVertex(p2, color);
        addVertex(p1, color);
        addVertex(p3, color);
        addVertex(p2, color);
	addDrawCall(GL_TRIANGLES, 6, modColor, 0, lw);
        break;
      }
      case CMD_quadT: {
        Vec2 p0 = fval2(itr), t0 = fval2(itr);
        Vec2 p1 = fval2(itr), t1 = fval2(itr);
        Vec2 p2 = fval2(itr), t2 = fval2(itr);
        Vec2 p3 = fval2(itr), t3 = fval2(itr);
	addVertex(p0, t0, color);
	addVertex(p1, t1, color);
	addVertex(p2, t2, color);
        addVertex(p1, t1, color);
        addVertex(p3, t3, color);
        addVertex(p2, t2, color);
	addDrawCall(GL_TRIANGLES, 6, modColor, tid, lw);
        break;
      }
      case CMD_quadC: {
        Vec2 p0 = fval2(itr); uint32_t c0 = uval(itr);
        Vec2 p1 = fval2(itr); uint32_t c1 = uval(itr);
        Vec2 p2 = fval2(itr); uint32_t c2 = uval(itr);
        Vec2 p3 = fval2(itr); uint32_t c3 = uval(itr);
        addVertex(p0, c0);
	addVertex(p1, c1);
	addVertex(p2, c2);
	addVertex(p1, c1);
	addVertex(p3, c3);
	addVertex(p2, c2);
	addDrawCall(GL_TRIANGLES, 6, modColor, 0, lw);
        break;
      }
      case CMD_quadTC: {
        Vec2 p0 = fval2(itr), t0 = fval2(itr); uint32_t c0 = uval(itr);
        Vec2 p1 = fval2(itr), t1 = fval2(itr); uint32_t c1 = uval(itr);
        Vec2 p2 = fval2(itr), t2 = fval2(itr); uint32_t c2 = uval(itr);
        Vec2 p3 = fval2(itr), t3 = fval2(itr); uint32_t c3 = uval(itr);
	addVertex(p0, t0, c0);
	addVertex(p1, t1, c1);
	addVertex(p2, t2, c2);
	addVertex(p1, t1, c1);
	addVertex(p3, t3, c3);
	addVertex(p2, t2, c2);
	addDrawCall(GL_TRIANGLES, 6, modColor, tid, lw);
        break;
      }
      case CMD_rectangle: {
        Vec2 p0 = fval2(itr), p3 = fval2(itr);
        Vec2 p1 = {p3.x,p0.y}, p2 = {p0.x,p3.y};
	addVertex(p0, color);
	addVertex(p1, color);
	addVertex(p2, color);
	addVertex(p1, color);
	addVertex(p3, color);
	addVertex(p2, color);
	addDrawCall(GL_TRIANGLES, 6, modColor, 0, lw);
	break;
      }
      case CMD_rectangleT: {
        Vec2 p0 = fval2(itr), t0 = fval2(itr);
        Vec2 p3 = fval2(itr), t3 = fval2(itr);
        Vec2 p1 = {p3.x,p0.y}, t1 = {t3.x,t0.y};
        Vec2 p2 = {p0.x,p3.y}, t2 = {t0.x,t3.y};
	addVertex(p0, t0, color);
	addVertex(p1, t1, color);
	addVertex(p2, t2, color);
	addVertex(p1, t1, color);
	addVertex(p3, t3, color);
	addVertex(p2, t2, color);
	addDrawCall(GL_TRIANGLES, 6, modColor, tid, lw);
	break;
      }
      default:
        itr = end; // stop processing at first invalid cmd
	break;
    }
  }
}

void gx::OpenGLRenderer::renderFrame()
{
  setCurrentContext(_window);
  GLCALL(glViewport, 0, 0, _width, _height);
  GLCALL(glClearColor, _bgColor.r, _bgColor.g, _bgColor.b, 0);
  GLCALL(glClear, GL_COLOR_BUFFER_BIT); // | GL_DEPTH_BUFFER_BIT

  GLCALL(glEnable, GL_BLEND);
  GLCALL(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  GLCALL(glDisable, GL_DEPTH_TEST);
  GLCALL(glEnable, GL_LINE_SMOOTH);

  // vbo/vao setup
  if (_changed) {
    _changed = false;
    if (!_drawCalls.empty()) {
      if (!_vbo) { _vbo.init(); }
      _vbo.setData(_vertices, GL_DYNAMIC_DRAW);

      if (!_vao) { _vao.init(); }
      constexpr std::size_t stride = sizeof(float)*4 + sizeof(uint32_t);
      _vao.enableAttrib(0); // vec4 (x,y,tx,ty)
      _vao.setAttrib(0, _vbo, 0, stride, 4, GL_FLOAT, GL_FALSE);
      _vao.enableAttrib(1); // uint (r,g,b,a packed int)
      //_vao.setAttrib(1, _vbo, sizeof(float)*4, stride, 4, GL_FLOAT, GL_FALSE);
      _vao.setAttribI(1, _vbo, sizeof(float)*4, stride, 1, GL_UNSIGNED_INT);
    } else {
      _vbo = GLBuffer();
      _vao = GLVertexArray();
    }
  }

  // simple ortho projection for 2d rendering in screen coords
  //  x:[0 width] => x:[-1 1]
  //  y:[0 height]   y:[-1 1]
  // origin in upper left corner
  Mat4 trans = {
    2.0f / float(_width), 0, 0, 0,
    0, -2.0f / float(_height), 0, 0,
    0, 0, 1, 0,
    -1, 1, 0, 1 };

  // clear texture unit assignments
  for (auto& t : _textures) { t.second.unit = -1; }
  GLuint nextTexUnit = 0;

  // draw
  _vao.bind();
  GLint first = 0;
  int lastShader = -1;
  float lastLineWidth = 0.0f;
  for (DrawCall& dc : _drawCalls) {
    GLuint texUnit = 0;
    int shader = 0;
    if (dc.texID > 0) {
      // shader uses texture - determine texture unit & bind if neccessary
      // (FIXME - no max texture units check currently)
      auto itr = _textures.find(dc.texID);
      if (itr != _textures.end()) {
	auto& [id,entry] = *itr;
	if (entry.unit < 0) {
	  entry.unit = nextTexUnit++;
	  entry.tex.bindUnit(entry.unit);
	}
	texUnit = entry.unit;
	shader = entry.shader;
      }
    }

    switch (shader) {
      case 0: // solid
	if (shader != lastShader) {
	  _sp0.use();
	  _sp0_trans.set(trans);
	}
        _sp0_modColor.set(dc.modColor);
	break;
      case 1: // mono texture
	if (shader != lastShader) {
	  _sp1.use();
	  _sp1_trans.set(trans);
	}
        _sp1_modColor.set(dc.modColor);
	_sp1_texUnit.set(texUnit);
	break;
      case 2: // full-color texture
	if (shader != lastShader) {
	  _sp2.use();
	  _sp2_trans.set(trans);
	}
        _sp2_modColor.set(dc.modColor);
	_sp2_texUnit.set(texUnit);
	break;
    }

    if (dc.mode == GL_LINES && dc.lineWidth != lastLineWidth) {
      GLCALL(glLineWidth, dc.lineWidth);
      lastLineWidth = dc.lineWidth;
    }

    GLCALL(glDrawArrays, dc.mode, first, dc.count);
    first += dc.count;
    lastShader = shader;
  }

  // swap buffers & finish
  glfwSwapBuffers(_window);
  GLCheckErrors("GL error");
  GLClearState();
}
