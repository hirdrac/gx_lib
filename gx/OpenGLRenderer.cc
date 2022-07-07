//
// gx/OpenGLRenderer.cc
// Copyright (C) 2022 Richard Bradley
//

// TODO: add blur transparency shader
// TODO: render thread
//   - thread for OpenGL, glfwMakeContextCurrent(), glfwGetProcAddress(),
//     glfwSwapInterval(), glfwSwapBuffers() calls

#include "OpenGLRenderer.hh"
#include "DrawLayer.hh"
#include "DrawEntry.hh"
#include "Image.hh"
#include "Color.hh"
#include "Projection.hh"
#include "Logger.hh"
#include "OpenGL.hh"
#include "Assert.hh"
#include <GLFW/glfw3.h>
#include <cstring>
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
  std::lock_guard lg{_glMutex};
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

  #undef UNIFORM_BLOCK_SRC

  // uniform location cache
  bool status = true;
  for (int i = 0; i < SHADER_COUNT; ++i) {
    GLProgram& p = _sp[i];
    status = status && p;
    p.setUniformBlockBinding(p.getUniformBlockIndex("ub0"), 0);
    _sp_texUnit[i] = p.getUniformLocation("texUnit");
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

TextureID OpenGLRenderer::setTexture(
  TextureID id, const Image& img, int levels,
  FilterType minFilter, FilterType magFilter)
{
  GLenum texformat, imgformat;
  switch (img.channels()) {
    case 1: texformat = GL_R8;    imgformat = GL_RED;  break;
    case 2: texformat = GL_RG8;   imgformat = GL_RG;   break;
    case 3: texformat = GL_RGB8;  imgformat = GL_RGB;  break;
    case 4: texformat = GL_RGBA8; imgformat = GL_RGBA; break;
    default: return 0;
  }

  bool newTexture = false;
  if (id <= 0) {
    id = newTextureID();
    newTexture = true;
  }

  std::lock_guard lg{_glMutex};
  TextureEntry* ePtr;
  if (newTexture) {
    ePtr = &_textures[id];
  } else {
    // update existing entry
    const auto itr = _textures.find(id);
    if (itr == _textures.end()) { return 0; }
    ePtr = &(itr->second);
  }

  setCurrentContext(_window);

  GLTexture2D& t = ePtr->tex;
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
    GLint val;
    if (levels <= 1) {
      val = (minFilter == FILTER_LINEAR) ? GL_LINEAR : GL_NEAREST;
    } else {
      val = (minFilter == FILTER_LINEAR)
        ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_LINEAR;
      // other values:
      // GL_NEAREST_MIPMAP_NEAREST
      // GL_LINEAR_MIPMAP_NEAREST
    }

    t.setParameter(GL_TEXTURE_MIN_FILTER, val);
  }

  if (magFilter != FILTER_UNSPECIFIED) {
    t.setParameter(GL_TEXTURE_MAG_FILTER,
                   (magFilter == FILTER_LINEAR) ? GL_LINEAR : GL_NEAREST);
  }

  return id;
}

void OpenGLRenderer::freeTexture(TextureID id)
{
  std::lock_guard lg{_glMutex};
  setCurrentContext(_window);
  _textures.erase(id);
}

void OpenGLRenderer::draw(
  int width, int height, std::initializer_list<DrawLayer*> dl)
{
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
        case CMD_line2C:       d += 7;  vsize += 2; break;
        case CMD_line3C:       d += 9;  vsize += 2; break;
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

    GX_ASSERT(d == data_end);
  }

  std::lock_guard lg{_glMutex};
  setCurrentContext(_window);

  _opData.clear();
  _lastOp = OP_null;
  _width = width;
  _height = height;

  if (vsize == 0) {
    _vbo = GLBuffer{};
    _vao = GLVertexArray{};
  } else if (!_vbo) {
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

  Vertex3NTC* ptr = nullptr;
  if (_vbo) {
    _vbo.setData(GLsizei(vsize * sizeof(Vertex3NTC)), nullptr, GL_STREAM_DRAW);
    ptr = static_cast<Vertex3NTC*>(_vbo.map(GL_WRITE_ONLY));
    GX_ASSERT(ptr != nullptr);
  }

  // general triangle layout
  //  0--1
  //  | /|
  //  |/ |
  //  2--3

  int32_t first = 0;
  for (const DrawLayer* lPtr : dl) {
    const bool firstLayer = (lPtr == *dl.begin());
    if (lPtr->transformSet) {
      addOp(OP_viewT, lPtr->view);
      addOp(OP_projT, lPtr->proj);
    } else if (firstLayer) {
      // default 2D projection
      addOp(OP_viewT, Mat4{INIT_IDENTITY});
      addOp(OP_projT, orthoProjection(float(_width), float(_height)));
    }

    if (lPtr->useLight) {
      addOp(OP_light, lPtr->lightPos.x, lPtr->lightPos.y, lPtr->lightPos.z,
            lPtr->lightA, lPtr->lightD);
    } else {
      addOp(OP_no_light);
    }

    if (lPtr->modColor != 0) {
      addOp(OP_modColor, lPtr->modColor);
    }

    if (lPtr->clearDepth && lPtr->bgColor != 0) {
      addOp(OP_bgColor, lPtr->bgColor);
      addOp(OP_clear_all);
    } else if (lPtr->clearDepth) {
      addOp(OP_clear_depth);
    } else if (lPtr->bgColor != 0) {
      addOp(OP_bgColor, lPtr->bgColor);
      addOp(OP_clear_color);
    }

    if (lPtr->cap >= 0) {
      addOp(OP_capabilities, lPtr->cap);
    } else if (firstLayer) {
      // set default initial capabilities
      addOp(OP_capabilities, BLEND);
    }

    uint32_t color = 0;
    TextureID tid = 0;
    Vec3 normal{0,0,0};

    const DrawEntry* data     = lPtr->entries.data();
    const DrawEntry* data_end = data + lPtr->entries.size();
    for (const DrawEntry* d = data; d != data_end; ) {
      const DrawCmd cmd = (d++)->cmd;
      switch (cmd) {
        case CMD_color:   color  = uval(d); break;
        case CMD_texture: tid    = uval(d); break;
        case CMD_normal3: normal = fval3(d); break;

        case CMD_lineWidth:
          addOp(OP_lineWidth, fval(d));
          break;

        case CMD_line2: {
          vertex2d(ptr, fval2(d), color);
          vertex2d(ptr, fval2(d), color);
          addDrawLines(first, 2);
          break;
        }
        case CMD_line3: {
          vertex3d(ptr, fval3(d), color);
          vertex3d(ptr, fval3(d), color);
          addDrawLines(first, 2);
          break;
        }
        case CMD_line2C: {
          const Vec2 p0 = fval2(d); const uint32_t c0 = uval(d);
          const Vec2 p1 = fval2(d); const uint32_t c1 = uval(d);
          vertex2d(ptr, p0, c0);
          vertex2d(ptr, p1, c1);
          addDrawLines(first, 2);
          break;
        }
        case CMD_line3C: {
          const Vec3 p0 = fval3(d); const uint32_t c0 = uval(d);
          const Vec3 p1 = fval3(d); const uint32_t c1 = uval(d);
          vertex3d(ptr, p0, c0);
          vertex3d(ptr, p1, c1);
          addDrawLines(first, 2);
          break;
        }
        case CMD_triangle2: {
          vertex2d(ptr, fval2(d), color);
          vertex2d(ptr, fval2(d), color);
          vertex2d(ptr, fval2(d), color);
          addDrawTriangles(first, 3, 0);
          break;
        }
        case CMD_triangle3: {
          vertex3d(ptr, fval3(d), normal, color);
          vertex3d(ptr, fval3(d), normal, color);
          vertex3d(ptr, fval3(d), normal, color);
          addDrawTriangles(first, 3, 0);
          break;
        }
        case CMD_triangle2T: {
          const Vec2 p0 = fval2(d), t0 = fval2(d);
          const Vec2 p1 = fval2(d), t1 = fval2(d);
          const Vec2 p2 = fval2(d), t2 = fval2(d);
          vertex2d(ptr, p0, t0, color);
          vertex2d(ptr, p1, t1, color);
          vertex2d(ptr, p2, t2, color);
          addDrawTriangles(first, 3, tid);
          break;
        }
        case CMD_triangle3T: {
          const Vec3 p0 = fval3(d); const Vec2 t0 = fval2(d);
          const Vec3 p1 = fval3(d); const Vec2 t1 = fval2(d);
          const Vec3 p2 = fval3(d); const Vec2 t2 = fval2(d);
          vertex3d(ptr, p0, normal, t0, color);
          vertex3d(ptr, p1, normal, t1, color);
          vertex3d(ptr, p2, normal, t2, color);
          addDrawTriangles(first, 3, tid);
          break;
        }
        case CMD_triangle2C: {
          const Vec2 p0 = fval2(d); const uint32_t c0 = uval(d);
          const Vec2 p1 = fval2(d); const uint32_t c1 = uval(d);
          const Vec2 p2 = fval2(d); const uint32_t c2 = uval(d);
          vertex2d(ptr, p0, c0);
          vertex2d(ptr, p1, c1);
          vertex2d(ptr, p2, c2);
          addDrawTriangles(first, 3, 0);
          break;
        }
        case CMD_triangle3C: {
          const Vec3 p0 = fval3(d); const uint32_t c0 = uval(d);
          const Vec3 p1 = fval3(d); const uint32_t c1 = uval(d);
          const Vec3 p2 = fval3(d); const uint32_t c2 = uval(d);
          vertex3d(ptr, p0, normal, c0);
          vertex3d(ptr, p1, normal, c1);
          vertex3d(ptr, p2, normal, c2);
          addDrawTriangles(first, 3, 0);
          break;
        }
        case CMD_triangle2TC: {
          const Vec2 p0 = fval2(d), t0 = fval2(d); const uint32_t c0 = uval(d);
          const Vec2 p1 = fval2(d), t1 = fval2(d); const uint32_t c1 = uval(d);
          const Vec2 p2 = fval2(d), t2 = fval2(d); const uint32_t c2 = uval(d);
          vertex2d(ptr, p0, t0, c0);
          vertex2d(ptr, p1, t1, c1);
          vertex2d(ptr, p2, t2, c2);
          addDrawTriangles(first, 3, tid);
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
          addDrawTriangles(first, 3, tid);
          break;
        }
        case CMD_triangle3NTC: {
          *ptr++ = vertex_val(d);
          *ptr++ = vertex_val(d);
          *ptr++ = vertex_val(d);
          addDrawTriangles(first, 3, tid);
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
          addDrawTriangles(first, 6, 0);
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
          addDrawTriangles(first, 6, 0);
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
          addDrawTriangles(first, 6, tid);
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
          addDrawTriangles(first, 6, tid);
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
          addDrawTriangles(first, 6, 0);
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
          addDrawTriangles(first, 6, 0);
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
          addDrawTriangles(first, 6, tid);
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
          addDrawTriangles(first, 6, tid);
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
          addDrawTriangles(first, 6, tid);
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
          addDrawTriangles(first, 6, 0);
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
          addDrawTriangles(first, 6, tid);
          break;
        }
        default:
          d = data_end; // stop processing at first invalid cmd
          break;
      }
    }
  }

  if (_vbo) { _vbo.unmap(); }

#if 0
  std::size_t dsize = 0;
  for (const DrawLayer* lPtr : dl) { dsize += lPtr->entries.size(); }
  println_err("entries:", dsize, "  vertices:", vsize,
              "  opData:", _opData.size());
#endif
}

void OpenGLRenderer::renderFrame()
{
  std::lock_guard lg{_glMutex};
  setCurrentContext(_window);

  GX_GLCALL(glViewport, 0, 0, _width, _height);
  GX_GLCALL(glClearDepth, 1.0);

  // clear texture unit assignments
  for (auto& t : _textures) { t.second.unit = -1; }

  if (_opData.empty()) { return; }

  GX_GLCALL(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  GX_GLCALL(glEnable, GL_LINE_SMOOTH);
  GX_GLCALL(glFrontFace, GL_CW);

  _currentGLCap = -1; // force all capabilities to be set initially
  _uniformBuf.bindBase(GL_UNIFORM_BUFFER, 0);

  bool udChanged = true;
  UniformData ud{};

  // draw
  _vao.bind();
  int lastShader = -1;
  int nextTexUnit = 0;
  int texUnit = -1;
  bool useLight = false;
  bool setUnit = false;

  const OpEntry* data     = _opData.data();
  const OpEntry* data_end = data + _opData.size();
  for (const OpEntry* d = data; d < data_end; ) {
    const GLOperation op = (d++)->op;
    switch (op) {
      case OP_viewT:
        std::memcpy(ud.viewT.data(), d, sizeof(float)*16); d += 16;
        udChanged = true;
        break;
      case OP_projT:
        std::memcpy(ud.projT.data(), d, sizeof(float)*16); d += 16;
        udChanged = true;
        break;
      case OP_modColor:
        ud.modColor = (d++)->uval;
        udChanged = true;
        break;
      case OP_light:
        std::memcpy(ud.lightPos.data(), d, sizeof(float)*3); d += 3;
        ud.lightA = (d++)->uval;
        ud.lightD = (d++)->uval;
        useLight = udChanged = true;
        break;
      case OP_no_light:
        useLight = false;
        break;
      case OP_capabilities:
        setGLCapabilities((d++)->ival);
        break;
      case OP_lineWidth:
        GX_GLCALL(glLineWidth, (d++)->fval);
        break;
      case OP_bgColor: {
        const Color c = unpackRGBA8((d++)->uval);
        GX_GLCALL(glClearColor, c.r, c.g, c.b, c.a);
        break;
      }
      case OP_clear_color:
        GX_GLCALL(glClear, GL_COLOR_BUFFER_BIT);
        break;
      case OP_clear_depth:
        GX_GLCALL(glClear, GL_DEPTH_BUFFER_BIT);
        break;
      case OP_clear_all:
        GX_GLCALL(glClear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        break;
      case OP_draw_lines: {
        const GLint first = (d++)->ival;
        const GLsizei count = (d++)->ival;
        if (udChanged) {
          _uniformBuf.setSubData(0, sizeof(ud), &ud);
          udChanged = false;
        }

        if (lastShader != 0) {
          lastShader = 0;
          _sp[0].use();
        }

        GX_GLCALL(glDrawArrays, GL_LINES, first, count);
        break;
      }
      case OP_draw_triangles: {
        const GLint first = (d++)->ival;
        const GLsizei count = (d++)->ival;
        const TextureID tid = (d++)->uval;
        if (udChanged) {
          _uniformBuf.setSubData(0, sizeof(ud), &ud);
          udChanged = false;
        }

        int shader = useLight ? 3 : 0;
        if (tid != 0) {
          // shader uses texture - determine texture unit & bind if necessary
          // (FIXME: no max texture units check currently)
          const auto itr = _textures.find(tid);
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
          lastShader = shader;
          _sp[shader].use();
          setUnit = bool(_sp_texUnit[shader]);
        }
        if (setUnit) { _sp_texUnit[shader].set(texUnit); }

        GX_GLCALL(glDrawArrays, GL_TRIANGLES, first, count);
        break;
      }
      default:
        GX_ASSERT(op == OP_null);
        break;
    }
  }

  // swap buffers & finish
  glfwSwapBuffers(_window);
  GLCheckErrors("GL error");
  GLClearState();
}

void OpenGLRenderer::setGLCapabilities(int cap)
{
  constexpr int CULL = CULL_CW | CULL_CCW;
  GX_ASSERT(cap >= 0);

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
