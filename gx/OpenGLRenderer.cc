//
// gx/OpenGLRenderer.cc
// Copyright (C) 2021 Richard Bradley
//

// TODO - add blur transparency shader
// TODO - render thread:
//   * thread for OpenGL, glfwMakeContextCurrent(), glfwGetProcAddress(),
//     glfwSwapInterval(), glfwSwapBuffers() calls

#include "OpenGLRenderer.hh"
#include "DrawEntry.hh"
#include "Image.hh"
#include "Color.hh"
#include "Camera.hh"
#include "Logger.hh"
#include "OpenGL.hh"
#include "Print.hh"
#include <GLFW/glfw3.h>
#include <cstdlib>
#include <cassert>

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

  [[nodiscard]] GLProgram makeProgram(const char* vsrc, const char* fsrc)
  {
    GLShader vshader;
    if (!vshader.init(GL_VERTEX_SHADER, vsrc, GLSL_SOURCE_HEADER)) {
      GX_LOG_ERROR("vshader error: ", vshader.infoLog());
      return GLProgram();
    }

    GLShader fshader;
    if (!fshader.init(GL_FRAGMENT_SHADER, fsrc, GLSL_SOURCE_HEADER)) {
      GX_LOG_ERROR("fshader error: ", fshader.infoLog());
      return GLProgram();
    }

    GLProgram prog;
    if (!prog.init(vshader, fshader)) {
      GX_LOG_ERROR("program link error: ", prog.infoLog());
      return GLProgram();
    }

    return prog;
  }

  [[nodiscard]] GLint getUniformLoc(const GLProgram& p, const char* name)
  {
    GLint loc = p.getUniformLocation(name);
    if (loc < 0) {
      GX_LOG_ERROR("unknown uniform location '", name, "' for prog ", p.id());
    }
    return loc;
  }

  // DrawEntry iterator reading helper functions
  inline uint32_t ival(const gx::DrawEntry*& ptr) {
    return (ptr++)->ival; }
  inline uint32_t uval(const gx::DrawEntry*& ptr) {
    return (ptr++)->uval; }
  inline float fval(const gx::DrawEntry*& ptr) {
    return (ptr++)->fval; }
  inline gx::Vec2 fval2(const gx::DrawEntry*& ptr) {
    return {fval(ptr), fval(ptr)}; }
  inline gx::Vec3 fval3(const gx::DrawEntry*& ptr) {
    return {fval(ptr), fval(ptr), fval(ptr)}; }
  inline gx::Mat4 mat4_val(const gx::DrawEntry*& ptr) {
    return {fval(ptr), fval(ptr), fval(ptr), fval(ptr),
            fval(ptr), fval(ptr), fval(ptr), fval(ptr),
            fval(ptr), fval(ptr), fval(ptr), fval(ptr),
            fval(ptr), fval(ptr), fval(ptr), fval(ptr)}; }

  // vertex output functions
  inline void vertex(gx::Vertex3TC*& ptr, gx::Vec2 pt, uint32_t c) {
    *ptr++ = {pt.x,pt.y,0.0f, 0.0f,0.0f, c}; }
  inline void vertex(gx::Vertex3TC*& ptr, gx::Vec3 pt, uint32_t c) {
    *ptr++ = {pt.x,pt.y,pt.z, 0.0f,0.0f, c}; }
  inline void vertex(gx::Vertex3TC*& ptr, gx::Vec2 pt, gx::Vec2 tx, uint32_t c) {
    *ptr++ = {pt.x,pt.y,0.0f, tx.x,tx.y, c}; }
  inline void vertex(gx::Vertex3TC*& ptr, gx::Vec3 pt, gx::Vec2 tx, uint32_t c) {
    *ptr++ = {pt.x,pt.y,pt.z, tx.x,tx.y, c}; }


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

  _maxTextureSize = GLTexture2D::maxSize();
  glfwSwapInterval(1); // enable V-SYNC

  // solid color shader
  static const char* SP0V_SRC =
    "layout(location = 0) in vec3 in_pos;" // x,y,z
    "layout(location = 2) in uint in_color;"
    "uniform mat4 trans;"
    "uniform uint modColor;"
    "out vec4 v_color;"
    "void main() {"
    "  v_color = unpackUnorm4x8(in_color) * unpackUnorm4x8(modColor);"
    "  gl_Position = trans * vec4(in_pos, 1);"
    "}";
  static const char* SP0F_SRC =
    "in vec4 v_color;"
    "out vec4 fragColor;"
    "void main() { fragColor = v_color; }";
  _sp[0] = makeProgram(SP0V_SRC, SP0F_SRC);

  // mono color texture shader (fonts)
  static const char* SP1V_SRC =
    "layout(location = 0) in vec3 in_pos;" // x,y,z
    "layout(location = 1) in vec2 in_tc;"  // s,t
    "layout(location = 2) in uint in_color;"
    "uniform mat4 trans;"
    "uniform uint modColor;"
    "out vec4 v_color;"
    "out vec2 v_texCoord;"
    "void main() {"
    "  v_color = unpackUnorm4x8(in_color) * unpackUnorm4x8(modColor);"
    "  v_texCoord = in_tc;"
    "  gl_Position = trans * vec4(in_pos, 1);"
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
  _sp[1] = makeProgram(SP1V_SRC, SP1F_SRC);

  // full color texture shader (images)
  static const char* SP2F_SRC =
    "in vec2 v_texCoord;"
    "in vec4 v_color;"
    "uniform sampler2D texUnit;"
    "out vec4 fragColor;"
    "void main() { fragColor = texture(texUnit, v_texCoord) * v_color; }";
  _sp[2] = makeProgram(SP1V_SRC, SP2F_SRC);

  // uniform location cache
  bool status = true;
  for (int i = 0; i < 3; ++i) {
    status = status && _sp[i];
    _sp_trans[i] = getUniformLoc(_sp[i], "trans");
    _sp_modColor[i] = getUniformLoc(_sp[i], "modColor");
    if (i > 0) {
      _sp_texUnit[i] = getUniformLoc(_sp[i], "texUnit");
    }
  }

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

  return status;
}

gx::TextureID gx::OpenGLRenderer::setTexture(
  TextureID id, const Image& img, int levels,
  FilterType minFilter, FilterType magFilter)
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
    t.init(std::max(1, levels), texformat, img.width(), img.height());
    ePtr->shader = (img.channels() == 1) ? 1 : 2;
  }

  t.setSubImage2D(
    0, 0, 0, img.width(), img.height(), imgformat, img.data());
  if (levels > 1) { t.generateMipmap(); }

  // TODO - make these configurable
  t.setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  t.setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // GL_CLAMP_TO_EDGE
    // GL_CLAMP_TO_BORDER
    // GL_MIRRORED_REPEAT
    // GL_REPEAT
    // GL_MIRROR_CLAMP_TO_EDGE

  if (minFilter != FILTER_UNSPECIFIED) {
    if (levels <= 1) {
      t.setParameter(GL_TEXTURE_MIN_FILTER,
                     (minFilter == FILTER_LINEAR) ? GL_LINEAR : GL_NEAREST);
    } else {
      t.setParameter(GL_TEXTURE_MIN_FILTER,
                     (minFilter == FILTER_LINEAR)
                     ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_LINEAR);
      // other values:
      // GL_NEAREST_MIPMAP_NEAREST
      // GL_LINEAR_MIPMAP_NEAREST
    }
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
  _drawBuffer.clear();
  _lastCap = -1;
  _changed = true;

  Mat4 trans;
  calcScreenOrthoProjection(float(width), float(height), trans);
  _transforms.clear();
  _transforms.push_back({Mat4Identity, trans});

  _drawCalls.clear();
}

void gx::OpenGLRenderer::setupBuffer()
{
  const DrawEntry* data = _drawBuffer.data();
  const DrawEntry* end  = data + _drawBuffer.size();

  unsigned int size = 0; // vertices needed
  for (const DrawEntry* itr = data; itr != end; ) {
    DrawCmd cmd = itr->cmd;
    switch (cmd) {
      case CMD_capabilities: itr += 2; break;
      case CMD_transform:    itr += 33; break;
      case CMD_color:        itr += 2; break;
      case CMD_modColor:     itr += 2; break;
      case CMD_texture:      itr += 2; break;
      case CMD_lineWidth:    itr += 2; break;
      case CMD_line2:        itr += 5;  size += 2; break;
      case CMD_line3:        itr += 7;  size += 2; break;
      case CMD_triangle2:    itr += 7;  size += 3; break;
      case CMD_triangle3:    itr += 10; size += 3; break;
      case CMD_triangle2T:   itr += 13; size += 3; break;
      case CMD_triangle3T:   itr += 16; size += 3; break;
      case CMD_triangle2C:   itr += 10; size += 3; break;
      case CMD_triangle3C:   itr += 13; size += 3; break;
      case CMD_triangle2TC:  itr += 16; size += 3; break;
      case CMD_triangle3TC:  itr += 19; size += 3; break;
      case CMD_quad2:        itr += 9;  size += 6; break;
      case CMD_quad3:        itr += 13; size += 6; break;
      case CMD_quad2T:       itr += 17; size += 6; break;
      case CMD_quad3T:       itr += 21; size += 6; break;
      case CMD_quad2C:       itr += 13; size += 6; break;
      case CMD_quad3C:       itr += 17; size += 6; break;
      case CMD_quad2TC:      itr += 21; size += 6; break;
      case CMD_quad3TC:      itr += 25; size += 6; break;
      case CMD_rectangle:    itr += 5;  size += 6; break;
      case CMD_rectangleT:   itr += 9;  size += 6; break;

      default:
        itr = end; // stop reading at first invalid cmd
        GX_LOG_ERROR("unknown DrawCmd value: ", int(cmd));
	break;
    }
  }

  if (size == 0) {
    _vbo = GLBuffer();
    _vao = GLVertexArray();
    return;
  }

  if (!_vbo) {
    _vbo.init();
    _vao.init();
    _vao.enableAttrib(0); // vec3 (x,y,z)
    _vao.setAttrib(0, _vbo, 0, 24, 3, GL_FLOAT, GL_FALSE);
    _vao.enableAttrib(1); // vec2 (s,t)
    _vao.setAttrib(1, _vbo, 12, 24, 2, GL_FLOAT, GL_FALSE);
    _vao.enableAttrib(2); // uint (r,g,b,a packed int)
    _vao.setAttribI(2, _vbo, 20, 24, 1, GL_UNSIGNED_INT);
  }

  _vbo.setData(GLsizei(size * sizeof(Vertex3TC)), nullptr, GL_STREAM_DRAW);
  Vertex3TC* ptr = (Vertex3TC*)_vbo.map(GL_WRITE_ONLY);
  assert(ptr != nullptr);

  // general triangle layout
  //  0--1
  //  | /|
  //  |/ |
  //  2--3

  uint32_t color = 0xffffffff;
  uint32_t modColor = 0xffffffff;
  TextureID tid = 0;
  float lw = 1.0f;
  int cap = 0;

  for (const DrawEntry* itr = data; itr != end; ) {
    DrawCmd cmd = (itr++)->cmd;
    switch (cmd) {
      case CMD_capabilities:
        cap = ival(itr);
        break;

      case CMD_transform: {
        Mat4 view = mat4_val(itr), proj = mat4_val(itr);
        if (!_transforms.empty()
            && (_drawCalls.empty()
                || _drawCalls.back().transformNo != (int(_transforms.size())-1)))
        {
          // last set transform not used for a draw call and can just be replaced
          _transforms.back() = {view, proj};
        } else {
          // add new transform entry
          _transforms.push_back({view, proj});
        }
        break;
      }

      case CMD_color:     color = uval(itr); break;
      case CMD_modColor:  modColor = uval(itr); break;
      case CMD_texture:   tid   = uval(itr); break;
      case CMD_lineWidth: lw    = fval(itr); break;

      case CMD_line2: {
	vertex(ptr, fval2(itr), color);
	vertex(ptr, fval2(itr), color);
	addDrawCall(2, GL_LINES, modColor, 0, lw, cap);
	break;
      }
      case CMD_line3: {
        vertex(ptr, fval3(itr), color);
        vertex(ptr, fval3(itr), color);
	addDrawCall(2, GL_LINES, modColor, 0, lw, cap);
        break;
      }
      case CMD_triangle2: {
	vertex(ptr, fval2(itr), color);
	vertex(ptr, fval2(itr), color);
	vertex(ptr, fval2(itr), color);
	addDrawCall(3, GL_TRIANGLES, modColor, 0, lw, cap);
	break;
      }
      case CMD_triangle3: {
	vertex(ptr, fval3(itr), color);
	vertex(ptr, fval3(itr), color);
	vertex(ptr, fval3(itr), color);
	addDrawCall(3, GL_TRIANGLES, modColor, 0, lw, cap);
	break;
      }
      case CMD_triangle2T: {
        Vec2 p0 = fval2(itr), t0 = fval2(itr);
        Vec2 p1 = fval2(itr), t1 = fval2(itr);
        Vec2 p2 = fval2(itr), t2 = fval2(itr);
        vertex(ptr, p0, t0, color);
	vertex(ptr, p1, t1, color);
	vertex(ptr, p2, t2, color);
	addDrawCall(3, GL_TRIANGLES, modColor, tid, lw, cap);
        break;
      }
      case CMD_triangle3T: {
        Vec3 p0 = fval3(itr); Vec2 t0 = fval2(itr);
        Vec3 p1 = fval3(itr); Vec2 t1 = fval2(itr);
        Vec3 p2 = fval3(itr); Vec2 t2 = fval2(itr);
        vertex(ptr, p0, t0, color);
	vertex(ptr, p1, t1, color);
	vertex(ptr, p2, t2, color);
	addDrawCall(3, GL_TRIANGLES, modColor, tid, lw, cap);
        break;
      }
      case CMD_triangle2C: {
        Vec2 p0 = fval2(itr); uint32_t c0 = uval(itr);
        Vec2 p1 = fval2(itr); uint32_t c1 = uval(itr);
        Vec2 p2 = fval2(itr); uint32_t c2 = uval(itr);
        vertex(ptr, p0, c0);
	vertex(ptr, p1, c1);
	vertex(ptr, p2, c2);
	addDrawCall(3, GL_TRIANGLES, modColor, 0, lw, cap);
        break;
      }
      case CMD_triangle3C: {
        Vec3 p0 = fval3(itr); uint32_t c0 = uval(itr);
        Vec3 p1 = fval3(itr); uint32_t c1 = uval(itr);
        Vec3 p2 = fval3(itr); uint32_t c2 = uval(itr);
        vertex(ptr, p0, c0);
	vertex(ptr, p1, c1);
	vertex(ptr, p2, c2);
	addDrawCall(3, GL_TRIANGLES, modColor, 0, lw, cap);
        break;
      }
      case CMD_triangle2TC: {
        Vec2 p0 = fval2(itr), t0 = fval2(itr); uint32_t c0 = uval(itr);
        Vec2 p1 = fval2(itr), t1 = fval2(itr); uint32_t c1 = uval(itr);
        Vec2 p2 = fval2(itr), t2 = fval2(itr); uint32_t c2 = uval(itr);
        vertex(ptr, p0, t0, c0);
	vertex(ptr, p1, t1, c1);
	vertex(ptr, p2, t2, c2);
	addDrawCall(3, GL_TRIANGLES, modColor, tid, lw, cap);
        break;
      }
      case CMD_triangle3TC: {
        Vec3 p0 = fval3(itr); Vec2 t0 = fval2(itr); uint32_t c0 = uval(itr);
        Vec3 p1 = fval3(itr); Vec2 t1 = fval2(itr); uint32_t c1 = uval(itr);
        Vec3 p2 = fval3(itr); Vec2 t2 = fval2(itr); uint32_t c2 = uval(itr);
        vertex(ptr, p0, t0, c0);
	vertex(ptr, p1, t1, c1);
	vertex(ptr, p2, t2, c2);
	addDrawCall(3, GL_TRIANGLES, modColor, tid, lw, cap);
        break;
      }
      case CMD_quad2: {
        Vec2 p0 = fval2(itr), p1 = fval2(itr);
        Vec2 p2 = fval2(itr), p3 = fval2(itr);
	vertex(ptr, p0, color);
	vertex(ptr, p1, color);
	vertex(ptr, p2, color);
        vertex(ptr, p1, color);
        vertex(ptr, p3, color);
        vertex(ptr, p2, color);
	addDrawCall(6, GL_TRIANGLES, modColor, 0, lw, cap);
        break;
      }
      case CMD_quad3: {
        Vec3 p0 = fval3(itr), p1 = fval3(itr);
        Vec3 p2 = fval3(itr), p3 = fval3(itr);
	vertex(ptr, p0, color);
	vertex(ptr, p1, color);
	vertex(ptr, p2, color);
        vertex(ptr, p1, color);
        vertex(ptr, p3, color);
        vertex(ptr, p2, color);
	addDrawCall(6, GL_TRIANGLES, modColor, 0, lw, cap);
        break;
      }
      case CMD_quad2T: {
        Vec2 p0 = fval2(itr), t0 = fval2(itr);
        Vec2 p1 = fval2(itr), t1 = fval2(itr);
        Vec2 p2 = fval2(itr), t2 = fval2(itr);
        Vec2 p3 = fval2(itr), t3 = fval2(itr);
	vertex(ptr, p0, t0, color);
	vertex(ptr, p1, t1, color);
	vertex(ptr, p2, t2, color);
        vertex(ptr, p1, t1, color);
        vertex(ptr, p3, t3, color);
        vertex(ptr, p2, t2, color);
	addDrawCall(6, GL_TRIANGLES, modColor, tid, lw, cap);
        break;
      }
      case CMD_quad3T: {
        Vec3 p0 = fval3(itr); Vec2 t0 = fval2(itr);
        Vec3 p1 = fval3(itr); Vec2 t1 = fval2(itr);
        Vec3 p2 = fval3(itr); Vec2 t2 = fval2(itr);
        Vec3 p3 = fval3(itr); Vec2 t3 = fval2(itr);
	vertex(ptr, p0, t0, color);
	vertex(ptr, p1, t1, color);
	vertex(ptr, p2, t2, color);
        vertex(ptr, p1, t1, color);
        vertex(ptr, p3, t3, color);
        vertex(ptr, p2, t2, color);
	addDrawCall(6, GL_TRIANGLES, modColor, tid, lw, cap);
        break;
      }
      case CMD_quad2C: {
        Vec2 p0 = fval2(itr); uint32_t c0 = uval(itr);
        Vec2 p1 = fval2(itr); uint32_t c1 = uval(itr);
        Vec2 p2 = fval2(itr); uint32_t c2 = uval(itr);
        Vec2 p3 = fval2(itr); uint32_t c3 = uval(itr);
        vertex(ptr, p0, c0);
	vertex(ptr, p1, c1);
	vertex(ptr, p2, c2);
	vertex(ptr, p1, c1);
	vertex(ptr, p3, c3);
	vertex(ptr, p2, c2);
	addDrawCall(6, GL_TRIANGLES, modColor, 0, lw, cap);
        break;
      }
      case CMD_quad3C: {
        Vec3 p0 = fval3(itr); uint32_t c0 = uval(itr);
        Vec3 p1 = fval3(itr); uint32_t c1 = uval(itr);
        Vec3 p2 = fval3(itr); uint32_t c2 = uval(itr);
        Vec3 p3 = fval3(itr); uint32_t c3 = uval(itr);
        vertex(ptr, p0, c0);
	vertex(ptr, p1, c1);
	vertex(ptr, p2, c2);
	vertex(ptr, p1, c1);
	vertex(ptr, p3, c3);
	vertex(ptr, p2, c2);
	addDrawCall(6, GL_TRIANGLES, modColor, 0, lw, cap);
        break;
      }
      case CMD_quad2TC: {
        Vec2 p0 = fval2(itr), t0 = fval2(itr); uint32_t c0 = uval(itr);
        Vec2 p1 = fval2(itr), t1 = fval2(itr); uint32_t c1 = uval(itr);
        Vec2 p2 = fval2(itr), t2 = fval2(itr); uint32_t c2 = uval(itr);
        Vec2 p3 = fval2(itr), t3 = fval2(itr); uint32_t c3 = uval(itr);
	vertex(ptr, p0, t0, c0);
	vertex(ptr, p1, t1, c1);
	vertex(ptr, p2, t2, c2);
	vertex(ptr, p1, t1, c1);
	vertex(ptr, p3, t3, c3);
	vertex(ptr, p2, t2, c2);
	addDrawCall(6, GL_TRIANGLES, modColor, tid, lw, cap);
        break;
      }
      case CMD_quad3TC: {
        Vec3 p0 = fval3(itr); Vec2 t0 = fval2(itr); uint32_t c0 = uval(itr);
        Vec3 p1 = fval3(itr); Vec2 t1 = fval2(itr); uint32_t c1 = uval(itr);
        Vec3 p2 = fval3(itr); Vec2 t2 = fval2(itr); uint32_t c2 = uval(itr);
        Vec3 p3 = fval3(itr); Vec2 t3 = fval2(itr); uint32_t c3 = uval(itr);
	vertex(ptr, p0, t0, c0);
	vertex(ptr, p1, t1, c1);
	vertex(ptr, p2, t2, c2);
	vertex(ptr, p1, t1, c1);
	vertex(ptr, p3, t3, c3);
	vertex(ptr, p2, t2, c2);
	addDrawCall(6, GL_TRIANGLES, modColor, tid, lw, cap);
        break;
      }
      case CMD_rectangle: {
        Vec2 p0 = fval2(itr), p3 = fval2(itr);
        Vec2 p1 = {p3.x,p0.y}, p2 = {p0.x,p3.y};
	vertex(ptr, p0, color);
	vertex(ptr, p1, color);
	vertex(ptr, p2, color);
	vertex(ptr, p1, color);
	vertex(ptr, p3, color);
	vertex(ptr, p2, color);
	addDrawCall(6, GL_TRIANGLES, modColor, 0, lw, cap);
	break;
      }
      case CMD_rectangleT: {
        Vec2 p0 = fval2(itr), t0 = fval2(itr);
        Vec2 p3 = fval2(itr), t3 = fval2(itr);
        Vec2 p1 = {p3.x,p0.y}, t1 = {t3.x,t0.y};
        Vec2 p2 = {p0.x,p3.y}, t2 = {t0.x,t3.y};
	vertex(ptr, p0, t0, color);
	vertex(ptr, p1, t1, color);
	vertex(ptr, p2, t2, color);
	vertex(ptr, p1, t1, color);
	vertex(ptr, p3, t3, color);
	vertex(ptr, p2, t2, color);
	addDrawCall(6, GL_TRIANGLES, modColor, tid, lw, cap);
	break;
      }
      default:
        itr = end; // stop processing at first invalid cmd
	break;
    }
  }

  _vbo.unmap();
#if 0
  println_err("drawBuffer:", _drawBuffer.size(), "  vertices:", size,
              "  drawCalls:", _drawCalls.size());
#endif
}

void gx::OpenGLRenderer::renderFrame()
{
  setCurrentContext(_window);
  GX_GLCALL(glViewport, 0, 0, _width, _height);
  GX_GLCALL(glClearColor, _bgColor.r, _bgColor.g, _bgColor.b, 0);
  GX_GLCALL(glClear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  _currentGLCap = -1; // force all capabilities to be set at first drawcall
  GX_GLCALL(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  GX_GLCALL(glEnable, GL_LINE_SMOOTH);

  // vbo/vao setup & draw call generation
  if (_changed) {
    _changed = false;
    setupBuffer();
  }

  // clear texture unit assignments
  for (auto& t : _textures) { t.second.unit = -1; }
  int nextTexUnit = 0;

  // draw
  _vao.bind();
  GLint first = 0;
  int lastShader = -1;
  float lastLineWidth = 0.0f;
  for (const DrawCall& dc : _drawCalls) {
    if (dc.capabilities != _currentGLCap) {
      setGLCapabilities(dc.capabilities);
    }

    int texUnit = 0;
    int shader = 0; // solid color shader
    if (dc.texID > 0) {
      // shader uses texture - determine texture unit & bind if neccessary
      // (FIXME - no max texture units check currently)
      auto itr = _textures.find(dc.texID);
      if (itr != _textures.end()) {
	auto& [id,entry] = *itr;
	if (entry.unit < 0) {
	  entry.unit = nextTexUnit++;
	  entry.tex.bindUnit(GLuint(entry.unit));
	}
	texUnit = entry.unit;
	shader = entry.shader; // mono or color texture shader (1 or 2)
      }
    }

    if (shader != lastShader) { _sp[shader].use(); }
    if (dc.transformNo >= 0) {
      const TransformEntry& t = _transforms[std::size_t(dc.transformNo)];
      _sp_trans[shader].set(t.view * t.projection);
    }
    _sp_modColor[shader].set(dc.modColor);
    if (_sp_texUnit[shader]) {_sp_texUnit[shader].set(texUnit); }

    if (dc.mode == GL_LINES && dc.lineWidth != lastLineWidth) {
      GX_GLCALL(glLineWidth, dc.lineWidth);
      lastLineWidth = dc.lineWidth;
    }

    GX_GLCALL(glDrawArrays, dc.mode, first, dc.count);
    first += dc.count;
    lastShader = shader;
  }

  // swap buffers & finish
  glfwSwapBuffers(_window);
  GLCheckErrors("GL error");
  GLClearState();
}

void gx::OpenGLRenderer::setGLCapabilities(int cap)
{
  if (_currentGLCap < 0)
  {
    // don't assume current state - enable/disable all values
    GX_GLCALL((cap & BLEND)      ? glEnable : glDisable, GL_BLEND);
    GX_GLCALL((cap & DEPTH_TEST) ? glEnable : glDisable, GL_DEPTH_TEST);
  }
  else
  {
    // enable/disable only for changes
    if (!(_currentGLCap & BLEND) && (cap & BLEND)) {
      GX_GLCALL(glEnable, GL_BLEND);
    } else if ((_currentGLCap & BLEND) && !(cap & BLEND)) {
      GX_GLCALL(glDisable, GL_BLEND);
    }

    if (!(_currentGLCap & DEPTH_TEST) && (cap & DEPTH_TEST)) {
      GX_GLCALL(glEnable, GL_DEPTH_TEST);
    } else if ((_currentGLCap & DEPTH_TEST) && !(cap & DEPTH_TEST)) {
      GX_GLCALL(glDisable, GL_DEPTH_TEST);
    }
  }

  _currentGLCap = cap;
}
