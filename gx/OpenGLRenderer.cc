//
// gx/OpenGLRenderer.cc
// Copyright (C) 2022 Richard Bradley
//

// TODO: add blur transparency shader
// TODO: render thread
//   - thread for OpenGL, glfwMakeContextCurrent(), glfwGetProcAddress(),
//     glfwSwapInterval(), glfwSwapBuffers() calls

#include "OpenGLRenderer.hh"
#include "DrawEntry.hh"
#include "Image.hh"
#include "Color.hh"
#include "Projection.hh"
#include "Logger.hh"
#include "OpenGL.hh"
//#include "Print.hh"
#include <GLFW/glfw3.h>
#include <cassert>
using namespace gx;


namespace {
  // **** Helper Functions ****
  void setCurrentContext(GLFWwindow* win)
  {
    // TODO: add lastThreadID check in debug
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
      return {};
    }

    GLShader fshader;
    if (!fshader.init(GL_FRAGMENT_SHADER, fsrc, GLSL_SOURCE_HEADER)) {
      GX_LOG_ERROR("fshader error: ", fshader.infoLog());
      return {};
    }

    GLProgram prog;
    if (!prog.init(vshader, fshader)) {
      GX_LOG_ERROR("program link error: ", prog.infoLog());
      return {};
    }

    return prog;
  }

  // DrawEntry iterator reading helper functions
  inline uint32_t uval(const DrawEntry*& ptr) {
    return (ptr++)->uval; }
  inline float fval(const DrawEntry*& ptr) {
    return (ptr++)->fval; }
  inline Vec2 fval2(const DrawEntry*& ptr) {
    return {fval(ptr), fval(ptr)}; }
  inline Vec3 fval3(const DrawEntry*& ptr) {
    return {fval(ptr), fval(ptr), fval(ptr)}; }

  inline Vertex3NTC vertex_val(const DrawEntry*& ptr) {
    return {fval(ptr), fval(ptr), fval(ptr),  // x,y,z
            fval(ptr), fval(ptr), fval(ptr),  // nx,ny,nz
            fval(ptr), fval(ptr), uval(ptr)}; // s,t,c
  }

#if 0
  // unused for now
  inline int32_t ival(const DrawEntry*& ptr) {
    return (ptr++)->ival; }
#endif

  // vertex output functions
  inline void vertex2d(Vertex3NTC*& ptr, Vec2 pt, uint32_t c) {
    *ptr++ = {pt.x,pt.y,0.0f, 0.0f,0.0f,1.0f, 0.0f,0.0f, c}; }
  inline void vertex2d(Vertex3NTC*& ptr, Vec2 pt, Vec2 tx, uint32_t c) {
    *ptr++ = {pt.x,pt.y,0.0f, 0.0f,0.0f,1.0f, tx.x,tx.y, c}; }

  inline void vertex3d(Vertex3NTC*& ptr, const Vec3& pt, uint32_t c) {
    *ptr++ = {pt.x,pt.y,pt.z, 0.0f,0.0f,0.0f, 0.0f,0.0f, c}; }
  inline void vertex3d(
    Vertex3NTC*& ptr, const Vec3& pt, const Vec3& n, uint32_t c) {
    *ptr++ = {pt.x,pt.y,pt.z, n.x,n.y,n.z, 0.0f,0.0f, c}; }
  inline void vertex3d(
    Vertex3NTC*& ptr, const Vec3& pt, const Vec3& n, Vec2 tx, uint32_t c) {
    *ptr++ = {pt.x,pt.y,pt.z, n.x,n.y,n.z, tx.x,tx.y, c}; }
}

void OpenGLRenderer::setWindowHints(bool debug)
{
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
  //glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, GL_VERSION_MAJOR);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, GL_VERSION_MINOR);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, debug ? GLFW_TRUE : GLFW_FALSE);
}

bool OpenGLRenderer::init(GLFWwindow* win)
{
  _window = win;
  setCurrentContext(win);
  if (!GLSetupContext(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
    return false;
  }

  _maxTextureSize = GLTexture2D::maxSize();
  glfwSwapInterval(1); // enable V-SYNC

  _uniformBuf.init(sizeof(UniformData), nullptr);
  #define UNIFORM_BLOCK_SRC\
    "layout(std140) uniform ub0 {"\
    "  mat4 viewT;"\
    "  mat4 projT;"\
    "  vec3 lightPos;"\
    "  uint lightA;"\
    "  uint lightD;"\
    "  uint modColor;"\
    "};"

  // solid color shader
  static const char* SP0V_SRC =
    "layout(location = 0) in vec3 in_pos;" // x,y,z
    "layout(location = 3) in uint in_color;"
    UNIFORM_BLOCK_SRC
    "out vec4 v_color;"
    "void main() {"
    "  v_color = unpackUnorm4x8(in_color) * unpackUnorm4x8(modColor);"
    "  gl_Position = projT * viewT * vec4(in_pos, 1);"
    "}";
  static const char* SP0F_SRC =
    "in vec4 v_color;"
    "out vec4 fragColor;"
    "void main() { fragColor = v_color; }";
  _sp[0] = makeProgram(SP0V_SRC, SP0F_SRC);

  // mono color texture shader (fonts)
  static const char* SP1V_SRC =
    "layout(location = 0) in vec3 in_pos;" // x,y,z
    "layout(location = 2) in vec2 in_tc;"  // s,t
    "layout(location = 3) in uint in_color;"
    UNIFORM_BLOCK_SRC
    "out vec4 v_color;"
    "out vec2 v_texCoord;"
    "void main() {"
    "  v_color = unpackUnorm4x8(in_color) * unpackUnorm4x8(modColor);"
    "  v_texCoord = in_tc;"
    "  gl_Position = projT * viewT * vec4(in_pos, 1);"
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

  // **** 3d shading w/ lighting ****
  static const char* SP3V_SRC =
    "layout(location = 0) in vec3 in_pos;"   // x,y,z
    "layout(location = 1) in vec3 in_norm;"  // nx,ny,nz
    "layout(location = 3) in uint in_color;"
    UNIFORM_BLOCK_SRC
    "out vec3 v_pos;"
    "out vec3 v_norm;"
    "out vec4 v_color;"
    "out vec3 v_lightPos;"
    "out vec3 v_lightA;"
    "out vec3 v_lightD;"
    "void main() {"
    "  v_pos = in_pos;"
    "  v_norm = in_norm;"
    "  v_color = unpackUnorm4x8(in_color) * unpackUnorm4x8(modColor);"
    "  v_lightPos = lightPos;"
    "  v_lightA = unpackUnorm4x8(lightA).rgb;"
    "  v_lightD = unpackUnorm4x8(lightD).rgb;"
    "  gl_Position = projT * viewT * vec4(in_pos, 1);"
    "}";
  static const char* SP3F_SRC =
    "in vec3 v_pos;"
    "in vec3 v_norm;"
    "in vec4 v_color;"
    "in vec3 v_lightPos;"
    "in vec3 v_lightA;"
    "in vec3 v_lightD;"
    "out vec4 fragColor;"
    "void main() {"
    "  vec3 lightDir = normalize(v_lightPos - v_pos);"
    "  float lt = max(dot(normalize(v_norm), lightDir), 0.0);"
    "  fragColor = v_color * vec4((v_lightD * lt) + v_lightA, 1.0);"
    "}";
    _sp[3] = makeProgram(SP3V_SRC, SP3F_SRC);

  // uniform location cache
  bool status = true;
  for (int i = 0; i < SHADER_COUNT; ++i) {
    GLProgram& p = _sp[i];
    status = status && p;
    p.setUniformBlockBinding(p.getUniformBlockIndex("ub0"), 0);
    _sp_texUnit[i] = p.getUniformLocation("texUnit");
  }

  #undef UNIFORM_BLOCK_SRC

#if 0
  // debug output
  float val[2] = {};
  glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, val);
  gx::println("GL_ALIASED_LINE_WIDTH_RANGE: ", val[0], " ", val[1]);

  glGetFloatv(GL_SMOOTH_LINE_WIDTH_RANGE, val);
  gx::println("GL_SMOOTH_LINE_WIDTH_RANGE: ", val[0], " ", val[1]);

  glGetFloatv(GL_SMOOTH_LINE_WIDTH_GRANULARITY, val);
  gx::println("GL_SMOOTH_LINE_WIDTH_GRANULARITY: ", val[0]);
#endif

  return status;
}

TextureID OpenGLRenderer::setTexture(
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
    id = newTextureID();
    ePtr = &_textures[id];
  } else {
    // update existing entry
    const auto itr = _textures.find(id);
    if (itr == _textures.end()) { return 0; }
    ePtr = &(itr->second);
  }

  GLTexture2D& t = ePtr->tex;
  setCurrentContext(_window);
  if (!t || t.width() != img.width() || t.height() != img.height()
      || t.internalFormat() != texformat) {
    t.init(std::max(1, levels), texformat, img.width(), img.height());
    ePtr->channels = img.channels();
  }

  t.setSubImage2D(
    0, 0, 0, img.width(), img.height(), imgformat, img.data());
  if (levels > 1) { t.generateMipmap(); }

  // TODO: make texture wrap params configurable
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

void OpenGLRenderer::draw(
  int width, int height, std::initializer_list<DrawLayer*> dl)
{
  _width = width;
  _height = height;

  unsigned int vsize = 0; // vertices needed
  for (const DrawLayer* lPtr : dl) {
    const DrawEntry* data     = lPtr->entries.data();
    const DrawEntry* data_end = data + lPtr->entries.size();

    const DrawEntry* d = data;
    while (d < data_end) {
      const DrawCmd cmd = d->cmd;
      switch (cmd) {
        case CMD_color:        d += 2; break;
        case CMD_texture:      d += 2; break;
        case CMD_lineWidth:    d += 2; break;
        case CMD_normal3:      d += 4; break;
        case CMD_line2:        d += 5;  vsize += 2; break;
        case CMD_line3:        d += 7;  vsize += 2; break;
        case CMD_triangle2:    d += 7;  vsize += 3; break;
        case CMD_triangle3:    d += 10; vsize += 3; break;
        case CMD_triangle2T:   d += 13; vsize += 3; break;
        case CMD_triangle3T:   d += 16; vsize += 3; break;
        case CMD_triangle2C:   d += 10; vsize += 3; break;
        case CMD_triangle3C:   d += 13; vsize += 3; break;
        case CMD_triangle2TC:  d += 16; vsize += 3; break;
        case CMD_triangle3TC:  d += 19; vsize += 3; break;
        case CMD_triangle3NTC: d += 28; vsize += 3; break;
        case CMD_quad2:        d += 9;  vsize += 6; break;
        case CMD_quad3:        d += 13; vsize += 6; break;
        case CMD_quad2T:       d += 17; vsize += 6; break;
        case CMD_quad3T:       d += 21; vsize += 6; break;
        case CMD_quad2C:       d += 13; vsize += 6; break;
        case CMD_quad3C:       d += 17; vsize += 6; break;
        case CMD_quad2TC:      d += 21; vsize += 6; break;
        case CMD_quad3TC:      d += 25; vsize += 6; break;
        case CMD_quad3NTC:     d += 37; vsize += 6; break;
        case CMD_rectangle:    d += 5;  vsize += 6; break;
        case CMD_rectangleT:   d += 9;  vsize += 6; break;

        default:
          d = data_end; // stop reading at first invalid cmd
          GX_LOG_ERROR("unknown DrawCmd value: ", int(cmd));
          break;
      }
    }

    assert(d == data_end);
  }

  _drawCalls.clear();
  if (vsize == 0) {
    _vbo = GLBuffer();
    _vao = GLVertexArray();

    if (dl.size() != 0) {
      // use state of first layer only
      addDrawCall(0, 0, 0, 0, *dl.begin());
    }
    return;
  }

  if (!_vbo) {
    _vbo.init();
    _vao.init();
    _vao.enableAttrib(0); // vec3 (x,y,z)
    _vao.setAttrib(0, _vbo, 0, 36, 3, GL_FLOAT, GL_FALSE);
    _vao.enableAttrib(1); // vec3 (nx,ny,nz)
    _vao.setAttrib(1, _vbo, 12, 36, 3, GL_FLOAT, GL_FALSE);
    _vao.enableAttrib(2); // vec2 (s,t)
    _vao.setAttrib(2, _vbo, 24, 36, 2, GL_FLOAT, GL_FALSE);
    _vao.enableAttrib(3); // uint (r,g,b,a packed int)
    _vao.setAttribI(3, _vbo, 32, 36, 1, GL_UNSIGNED_INT);
  }

  _vbo.setData(GLsizei(vsize * sizeof(Vertex3NTC)), nullptr, GL_STREAM_DRAW);
  Vertex3NTC* ptr = static_cast<Vertex3NTC*>(_vbo.map(GL_WRITE_ONLY));
  assert(ptr != nullptr);

  // general triangle layout
  //  0--1
  //  | /|
  //  |/ |
  //  2--3

  for (const DrawLayer* lPtr : dl) {
    if (lPtr->entries.empty()) {
      // add dummy drawcall for state changes of layer
      addDrawCall(0, 0, 0, 0, lPtr);
      continue;
    }

    uint32_t color = 0;
    TextureID tid = 0;
    float lw = 1.0f;
    Vec3 normal{0,0,0};

    const DrawEntry* data     = lPtr->entries.data();
    const DrawEntry* data_end = data + lPtr->entries.size();
    for (const DrawEntry* d = data; d != data_end; ) {
      const DrawCmd cmd = (d++)->cmd;
      switch (cmd) {
        case CMD_color:     color = uval(d); break;
        case CMD_texture:   tid   = uval(d); break;
        case CMD_lineWidth: lw    = fval(d); break;

        case CMD_normal3: normal = fval3(d); break;

        case CMD_line2: {
          vertex2d(ptr, fval2(d), color);
          vertex2d(ptr, fval2(d), color);
          addDrawCall(2, GL_LINES, 0, lw, lPtr);
          break;
        }
        case CMD_line3: {
          vertex3d(ptr, fval3(d), color);
          vertex3d(ptr, fval3(d), color);
          addDrawCall(2, GL_LINES, 0, lw, lPtr);
          break;
        }
        case CMD_triangle2: {
          vertex2d(ptr, fval2(d), color);
          vertex2d(ptr, fval2(d), color);
          vertex2d(ptr, fval2(d), color);
          addDrawCall(3, GL_TRIANGLES, 0, lw, lPtr);
          break;
        }
        case CMD_triangle3: {
          vertex3d(ptr, fval3(d), normal, color);
          vertex3d(ptr, fval3(d), normal, color);
          vertex3d(ptr, fval3(d), normal, color);
          addDrawCall(3, GL_TRIANGLES, 0, lw, lPtr);
          break;
        }
        case CMD_triangle2T: {
          const Vec2 p0 = fval2(d), t0 = fval2(d);
          const Vec2 p1 = fval2(d), t1 = fval2(d);
          const Vec2 p2 = fval2(d), t2 = fval2(d);
          vertex2d(ptr, p0, t0, color);
          vertex2d(ptr, p1, t1, color);
          vertex2d(ptr, p2, t2, color);
          addDrawCall(3, GL_TRIANGLES, tid, lw, lPtr);
          break;
        }
        case CMD_triangle3T: {
          const Vec3 p0 = fval3(d); const Vec2 t0 = fval2(d);
          const Vec3 p1 = fval3(d); const Vec2 t1 = fval2(d);
          const Vec3 p2 = fval3(d); const Vec2 t2 = fval2(d);
          vertex3d(ptr, p0, normal, t0, color);
          vertex3d(ptr, p1, normal, t1, color);
          vertex3d(ptr, p2, normal, t2, color);
          addDrawCall(3, GL_TRIANGLES, tid, lw, lPtr);
          break;
        }
        case CMD_triangle2C: {
          const Vec2 p0 = fval2(d); const uint32_t c0 = uval(d);
          const Vec2 p1 = fval2(d); const uint32_t c1 = uval(d);
          const Vec2 p2 = fval2(d); const uint32_t c2 = uval(d);
          vertex2d(ptr, p0, c0);
          vertex2d(ptr, p1, c1);
          vertex2d(ptr, p2, c2);
          addDrawCall(3, GL_TRIANGLES, 0, lw, lPtr);
          break;
        }
        case CMD_triangle3C: {
          const Vec3 p0 = fval3(d); const uint32_t c0 = uval(d);
          const Vec3 p1 = fval3(d); const uint32_t c1 = uval(d);
          const Vec3 p2 = fval3(d); const uint32_t c2 = uval(d);
          vertex3d(ptr, p0, normal, c0);
          vertex3d(ptr, p1, normal, c1);
          vertex3d(ptr, p2, normal, c2);
          addDrawCall(3, GL_TRIANGLES, 0, lw, lPtr);
          break;
        }
        case CMD_triangle2TC: {
          const Vec2 p0 = fval2(d), t0 = fval2(d); const uint32_t c0 = uval(d);
          const Vec2 p1 = fval2(d), t1 = fval2(d); const uint32_t c1 = uval(d);
          const Vec2 p2 = fval2(d), t2 = fval2(d); const uint32_t c2 = uval(d);
          vertex2d(ptr, p0, t0, c0);
          vertex2d(ptr, p1, t1, c1);
          vertex2d(ptr, p2, t2, c2);
          addDrawCall(3, GL_TRIANGLES, tid, lw, lPtr);
          break;
        }
        case CMD_triangle3TC: {
          const Vec3 p0 = fval3(d); const Vec2 t0 = fval2(d);
          const uint32_t c0 = uval(d);
          const Vec3 p1 = fval3(d); const Vec2 t1 = fval2(d);
          const uint32_t c1 = uval(d);
          const Vec3 p2 = fval3(d); const Vec2 t2 = fval2(d);
          const uint32_t c2 = uval(d);
          vertex3d(ptr, p0, normal, t0, c0);
          vertex3d(ptr, p1, normal, t1, c1);
          vertex3d(ptr, p2, normal, t2, c2);
          addDrawCall(3, GL_TRIANGLES, tid, lw, lPtr);
          break;
        }
        case CMD_triangle3NTC: {
          *ptr++ = vertex_val(d);
          *ptr++ = vertex_val(d);
          *ptr++ = vertex_val(d);
          break;
        }
        case CMD_quad2: {
          const Vec2 p0 = fval2(d), p1 = fval2(d);
          const Vec2 p2 = fval2(d), p3 = fval2(d);
          vertex2d(ptr, p0, color);
          vertex2d(ptr, p1, color);
          vertex2d(ptr, p2, color);
          vertex2d(ptr, p1, color);
          vertex2d(ptr, p3, color);
          vertex2d(ptr, p2, color);
          addDrawCall(6, GL_TRIANGLES, 0, lw, lPtr);
          break;
        }
        case CMD_quad3: {
          const Vec3 p0 = fval3(d), p1 = fval3(d);
          const Vec3 p2 = fval3(d), p3 = fval3(d);
          vertex3d(ptr, p0, normal, color);
          vertex3d(ptr, p1, normal, color);
          vertex3d(ptr, p2, normal, color);
          vertex3d(ptr, p1, normal, color);
          vertex3d(ptr, p3, normal, color);
          vertex3d(ptr, p2, normal, color);
          addDrawCall(6, GL_TRIANGLES, 0, lw, lPtr);
          break;
        }
        case CMD_quad2T: {
          const Vec2 p0 = fval2(d), t0 = fval2(d);
          const Vec2 p1 = fval2(d), t1 = fval2(d);
          const Vec2 p2 = fval2(d), t2 = fval2(d);
          const Vec2 p3 = fval2(d), t3 = fval2(d);
          vertex2d(ptr, p0, t0, color);
          vertex2d(ptr, p1, t1, color);
          vertex2d(ptr, p2, t2, color);
          vertex2d(ptr, p1, t1, color);
          vertex2d(ptr, p3, t3, color);
          vertex2d(ptr, p2, t2, color);
          addDrawCall(6, GL_TRIANGLES, tid, lw, lPtr);
          break;
        }
        case CMD_quad3T: {
          const Vec3 p0 = fval3(d); const Vec2 t0 = fval2(d);
          const Vec3 p1 = fval3(d); const Vec2 t1 = fval2(d);
          const Vec3 p2 = fval3(d); const Vec2 t2 = fval2(d);
          const Vec3 p3 = fval3(d); const Vec2 t3 = fval2(d);
          vertex3d(ptr, p0, normal, t0, color);
          vertex3d(ptr, p1, normal, t1, color);
          vertex3d(ptr, p2, normal, t2, color);
          vertex3d(ptr, p1, normal, t1, color);
          vertex3d(ptr, p3, normal, t3, color);
          vertex3d(ptr, p2, normal, t2, color);
          addDrawCall(6, GL_TRIANGLES, tid, lw, lPtr);
          break;
        }
        case CMD_quad2C: {
          const Vec2 p0 = fval2(d); const uint32_t c0 = uval(d);
          const Vec2 p1 = fval2(d); const uint32_t c1 = uval(d);
          const Vec2 p2 = fval2(d); const uint32_t c2 = uval(d);
          const Vec2 p3 = fval2(d); const uint32_t c3 = uval(d);
          vertex2d(ptr, p0, c0);
          vertex2d(ptr, p1, c1);
          vertex2d(ptr, p2, c2);
          vertex2d(ptr, p1, c1);
          vertex2d(ptr, p3, c3);
          vertex2d(ptr, p2, c2);
          addDrawCall(6, GL_TRIANGLES, 0, lw, lPtr);
          break;
        }
        case CMD_quad3C: {
          const Vec3 p0 = fval3(d); const uint32_t c0 = uval(d);
          const Vec3 p1 = fval3(d); const uint32_t c1 = uval(d);
          const Vec3 p2 = fval3(d); const uint32_t c2 = uval(d);
          const Vec3 p3 = fval3(d); const uint32_t c3 = uval(d);
          vertex3d(ptr, p0, normal, c0);
          vertex3d(ptr, p1, normal, c1);
          vertex3d(ptr, p2, normal, c2);
          vertex3d(ptr, p1, normal, c1);
          vertex3d(ptr, p3, normal, c3);
          vertex3d(ptr, p2, normal, c2);
          addDrawCall(6, GL_TRIANGLES, 0, lw, lPtr);
          break;
        }
        case CMD_quad2TC: {
          const Vec2 p0 = fval2(d), t0 = fval2(d); const uint32_t c0 = uval(d);
          const Vec2 p1 = fval2(d), t1 = fval2(d); const uint32_t c1 = uval(d);
          const Vec2 p2 = fval2(d), t2 = fval2(d); const uint32_t c2 = uval(d);
          const Vec2 p3 = fval2(d), t3 = fval2(d); const uint32_t c3 = uval(d);
          vertex2d(ptr, p0, t0, c0);
          vertex2d(ptr, p1, t1, c1);
          vertex2d(ptr, p2, t2, c2);
          vertex2d(ptr, p1, t1, c1);
          vertex2d(ptr, p3, t3, c3);
          vertex2d(ptr, p2, t2, c2);
          addDrawCall(6, GL_TRIANGLES, tid, lw, lPtr);
          break;
        }
        case CMD_quad3TC: {
          const Vec3 p0 = fval3(d); const Vec2 t0 = fval2(d);
          const uint32_t c0 = uval(d);
          const Vec3 p1 = fval3(d); const Vec2 t1 = fval2(d);
          const uint32_t c1 = uval(d);
          const Vec3 p2 = fval3(d); const Vec2 t2 = fval2(d);
          const uint32_t c2 = uval(d);
          const Vec3 p3 = fval3(d); const Vec2 t3 = fval2(d);
          const uint32_t c3 = uval(d);
          vertex3d(ptr, p0, normal, t0, c0);
          vertex3d(ptr, p1, normal, t1, c1);
          vertex3d(ptr, p2, normal, t2, c2);
          vertex3d(ptr, p1, normal, t1, c1);
          vertex3d(ptr, p3, normal, t3, c3);
          vertex3d(ptr, p2, normal, t2, c2);
          addDrawCall(6, GL_TRIANGLES, tid, lw, lPtr);
          break;
        }
        case CMD_quad3NTC: {
          const Vertex3NTC v0 = vertex_val(d);
          const Vertex3NTC v1 = vertex_val(d);
          const Vertex3NTC v2 = vertex_val(d);
          const Vertex3NTC v3 = vertex_val(d);
          *ptr++ = v0;
          *ptr++ = v1;
          *ptr++ = v2;
          *ptr++ = v1;
          *ptr++ = v3;
          *ptr++ = v2;
          break;
        }
        case CMD_rectangle: {
          const Vec2 p0 = fval2(d), p3 = fval2(d);
          const Vec2 p1{p3.x,p0.y}, p2{p0.x,p3.y};
          vertex2d(ptr, p0, color);
          vertex2d(ptr, p1, color);
          vertex2d(ptr, p2, color);
          vertex2d(ptr, p1, color);
          vertex2d(ptr, p3, color);
          vertex2d(ptr, p2, color);
          addDrawCall(6, GL_TRIANGLES, 0, lw, lPtr);
          break;
        }
        case CMD_rectangleT: {
          const Vec2 p0 = fval2(d), t0 = fval2(d);
          const Vec2 p3 = fval2(d), t3 = fval2(d);
          const Vec2 p1{p3.x,p0.y}, t1{t3.x,t0.y};
          const Vec2 p2{p0.x,p3.y}, t2{t0.x,t3.y};
          vertex2d(ptr, p0, t0, color);
          vertex2d(ptr, p1, t1, color);
          vertex2d(ptr, p2, t2, color);
          vertex2d(ptr, p1, t1, color);
          vertex2d(ptr, p3, t3, color);
          vertex2d(ptr, p2, t2, color);
          addDrawCall(6, GL_TRIANGLES, tid, lw, lPtr);
          break;
        }
        default:
          d = data_end; // stop processing at first invalid cmd
          break;
      }
    }
  }

  _vbo.unmap();

#if 0
  std::size_t dsize = 0;
  for (const DrawLayer* lPtr : dl) { dsize += lPtr->entries.size(); }
  gx::println_err("entries:", dsize, "  vertices:", vsize,
                  "  drawCalls:", _drawCalls.size());
#endif
}

void OpenGLRenderer::renderFrame()
{
  setCurrentContext(_window);
  GX_GLCALL(glViewport, 0, 0, _width, _height);
  GX_GLCALL(glClearDepth, 1.0);

  // clear texture unit assignments
  for (auto& t : _textures) { t.second.unit = -1; }

  if (_drawCalls.empty()) { return; }
  const DrawCall& firstCall = _drawCalls.front();

  GX_GLCALL(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  GX_GLCALL(glEnable, GL_LINE_SMOOTH);
  GX_GLCALL(glFrontFace, GL_CW);

  _currentGLCap = -1; // force all capabilities to be set at first drawcall
  if (firstCall.layerPtr->cap == -1) {
    setGLCapabilities(BLEND);
  }

  _uniformBuf.bindBase(GL_UNIFORM_BUFFER, 0);

  bool udChanged = true;
  UniformData ud{};

  if (!firstCall.layerPtr->transformSet) {
    // default 2D projection
    ud.viewT = Mat4Identity;
    ud.projT = orthoProjection(float(_width), float(_height));
  }

  // draw
  _vao.bind();
  GLint first = 0;
  int lastShader = -1;
  float lastLineWidth = 0.0f;
  int nextTexUnit = 0;
  const DrawLayer* lastLayer = nullptr;
  int texUnit = -1;

  for (const DrawCall& call : _drawCalls) {
    const DrawLayer* lPtr = call.layerPtr;
    if (lPtr->cap >= 0) { setGLCapabilities(lPtr->cap); }

    bool setUnit = false;
    int shader = lPtr->useLight ? 3 : 0; // solid color shader
    if (call.texID != 0) {
      // shader uses texture - determine texture unit & bind if necessary
      // (FIXME: no max texture units check currently)
      const auto itr = _textures.find(call.texID);
      if (itr != _textures.end()) {
	auto& [id,entry] = *itr;
	if (entry.unit < 0) {
	  entry.unit = nextTexUnit++;
	  entry.tex.bindUnit(GLuint(entry.unit));
	}
        setUnit = (entry.unit != texUnit);
        texUnit = entry.unit;
        // set mono or color texture shader
        shader = (entry.channels == 1) ? 1 : 2;
      }
    }

    // shader setup
    if (shader != lastShader) {
      _sp[shader].use();
      setUnit = bool(_sp_texUnit[shader]);
    }
    if (setUnit) { _sp_texUnit[shader].set(texUnit); }

    // uniform block update
    if (lastLayer != lPtr) {
      lastLayer = lPtr;

      GLbitfield clearMask = 0;
      if (lPtr->bgColor != 0) {
        const Color c = unpackRGBA8(lPtr->bgColor);
        GX_GLCALL(glClearColor, c.r, c.g, c.b, c.a);
        clearMask |= GL_COLOR_BUFFER_BIT;
      }

      if (lPtr->clearDepth) {
        clearMask |= GL_DEPTH_BUFFER_BIT;
      }

      if (clearMask != 0) {
        GX_GLCALL(glClear, clearMask);
      }

      // layer specific uniform data
      if (lPtr->transformSet) {
        ud.viewT = lPtr->view;
        ud.projT = lPtr->proj;
        udChanged = true;
      }

      if (lPtr->useLight) {
        ud.lightPos = lPtr->lightPos;
        ud.lightA = lPtr->lightA;
        ud.lightD = lPtr->lightD;
        udChanged = true;
      }

      if (lPtr->modColor != ud.modColor) {
        ud.modColor = lPtr->modColor;
        udChanged = true;
      }

      if (udChanged) {
        _uniformBuf.setSubData(0, sizeof(ud), &ud);
        udChanged = false;
      }
    }

    if (call.count != 0) {
      // line settings
      if (call.mode == GL_LINES && call.lineWidth != lastLineWidth) {
        GX_GLCALL(glLineWidth, call.lineWidth);
        lastLineWidth = call.lineWidth;
      }

      // draw call
      GX_GLCALL(glDrawArrays, call.mode, first, call.count);
      first += call.count;
    }

    lastShader = shader;
  }

  // swap buffers & finish
  glfwSwapBuffers(_window);
  GLCheckErrors("GL error");
  GLClearState();
}

void OpenGLRenderer::setGLCapabilities(int cap)
{
  constexpr int CULL = CULL_CW | CULL_CCW;
  assert(cap >= 0);

  if (_currentGLCap < 0)
  {
    // don't assume current state - enable/disable all values
    GX_GLCALL((cap & BLEND)      ? glEnable : glDisable, GL_BLEND);
    GX_GLCALL((cap & DEPTH_TEST) ? glEnable : glDisable, GL_DEPTH_TEST);
    if (cap & CULL) {
      GX_GLCALL(glEnable, GL_CULL_FACE);
      setCullFace(cap);
    } else {
      GX_GLCALL(glDisable, GL_CULL_FACE);
    }
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

    if (!(_currentGLCap & CULL) && (cap & CULL)) {
      GX_GLCALL(glEnable, GL_CULL_FACE);
    } else if ((_currentGLCap & CULL) && !(cap & CULL)) {
      GX_GLCALL(glDisable, GL_CULL_FACE);
    }

    if ((cap & CULL) && ((_currentGLCap & CULL) != (cap & CULL))) {
      setCullFace(cap);
    }
  }

  _currentGLCap = cap;
}

void OpenGLRenderer::setCullFace(int cap)
{
  const bool cw = cap & CULL_CW;
  const bool ccw = cap & CULL_CCW;
  // front face set to clockwise in renderFrame()
  if (cw && ccw) {
    GX_GLCALL(glCullFace, GL_FRONT_AND_BACK);
  } else if (cw) {
    GX_GLCALL(glCullFace, GL_FRONT);
  } else if (ccw) {
    GX_GLCALL(glCullFace, GL_BACK);
  }
}
