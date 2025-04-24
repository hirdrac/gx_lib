//
// gx/OpenGLRenderer.cc
// Copyright (C) 2025 Richard Bradley
//

// TODO: add blur transparency shader
// TODO: render thread
//   - thread for OpenGL, glfwMakeContextCurrent(), glfwGetProcAddress(),
//     glfwSwapInterval(), glfwSwapBuffers() calls
// TODO: init param to determine which shaders to create
// TODO: SDF glyph shader
// TODO: combine viewT & projT for CMD_camera?

#include "OpenGLRenderer.hh"
#include "DrawList.hh"
#include "DrawEntry.hh"
#include "Image.hh"
#include "Color.hh"
#include "Logger.hh"
#include "GLProgram.hh"
#include "GLBuffer.hh"
#include "GLVertexArray.hh"
#include "GLUniform.hh"
#include "GLTexture.hh"
#include "GLFramebuffer.hh"
#include "GLRenderbuffer.hh"
#include "OpenGL.hh"
#include "Assert.hh"
#include "Print.hh"
#include "GLFW.hh"
#include "Time.hh"
#include <vector>
#include <unordered_map>
#include <mutex>
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

  [[nodiscard]] const char* getGLString(GLenum name)
  {
    const char* val = reinterpret_cast<const char*>(glGetString(name));
    return val ? val : "ERROR";
  }

  template<int VER>
  [[nodiscard]] constexpr const char* shaderHeader()
  {
    if constexpr (VER < 42) {
      return "#version 330 core\n"
        "#extension GL_ARB_shading_language_packing : require\n";
    } else if constexpr (VER < 43) {
      return "#version 420 core\n";
    } else if constexpr (VER < 45) {
      return "#version 430 core\n";
    } else {
      return "#version 450 core\n";
    }
  }

  template<int VER>
  [[nodiscard]] GLShader makeVertexShader(const char* src)
  {
    GLShader vshader;
    if (!vshader.init(GL_VERTEX_SHADER, shaderHeader<VER>(), src)) {
      GX_LOG_ERROR("vertex shader error: ", vshader.infoLog());
      GX_LOG_ERROR("shader src: ", src);
      return {};
    }
    return vshader;
  }

  template<int VER>
  [[nodiscard]] GLShader makeFragmentShader(const char* src)
  {
    GLShader fshader;
    if (!fshader.init(GL_FRAGMENT_SHADER, shaderHeader<VER>(), src)) {
      GX_LOG_ERROR("fragment shader error: ", fshader.infoLog());
      GX_LOG_ERROR("shader src: ", src);
      return {};
    }
    return fshader;
  }

  template<int VER, class... Shader>
  [[nodiscard]] GLProgram makeProgram(const Shader&... shaders)
  {
    if (!(shaders && ...)) { return {}; }

    GLProgram prog;
    if (!prog.init(shaders...)) {
      GX_LOG_ERROR("program link error: ", prog.infoLog());
      return {};
    }
    return prog;
  }

  [[nodiscard]] std::size_t calcVSize(const DrawList& dl)
  {
    std::size_t vsize = 0;
    const Value* d    = dl.data();
    const Value* dEnd = d + dl.size();

    while (d < dEnd) {
      const uint32_t cmd = d->uval;
      switch (cmd) {
        case CMD_viewport:     d += 5; break;
        case CMD_viewportFull: d += 1; break;
        case CMD_color:        d += 2; break;
        case CMD_texture:      d += 2; break;
        case CMD_lineWidth:    d += 2; break;
        case CMD_normal:       d += 2; break;
        case CMD_modColor:     d += 2; break;
        case CMD_capabilities: d += 2; break;
        case CMD_camera:       d += 33; break;
        case CMD_cameraReset:  d += 1; break;
        case CMD_light:        d += 6; break;
        case CMD_clearView:    d += 2; break;
        case CMD_line2:        d += 5;  vsize += 2; break;
        case CMD_line3:        d += 7;  vsize += 2; break;
        case CMD_line2C:       d += 7;  vsize += 2; break;
        case CMD_line3C:       d += 9;  vsize += 2; break;
        case CMD_lineStart2:   d += 3;  break;
        case CMD_lineTo2:      d += 3;  vsize += 2; break;
        case CMD_lineStart3:   d += 4;  break;
        case CMD_lineTo3:      d += 4;  vsize += 2; break;
        case CMD_lineStart2C:  d += 4;  break;
        case CMD_lineTo2C:     d += 4;  vsize += 2; break;
        case CMD_lineStart3C:  d += 5;  break;
        case CMD_lineTo3C:     d += 5;  vsize += 2; break;
        case CMD_triangle2:    d += 7;  vsize += 3; break;
        case CMD_triangle3:    d += 10; vsize += 3; break;
        case CMD_triangle2T:   d += 13; vsize += 3; break;
        case CMD_triangle3T:   d += 16; vsize += 3; break;
        case CMD_triangle2C:   d += 10; vsize += 3; break;
        case CMD_triangle3C:   d += 13; vsize += 3; break;
        case CMD_triangle2TC:  d += 16; vsize += 3; break;
        case CMD_triangle3TC:  d += 19; vsize += 3; break;
        case CMD_triangle3TCN: d += 22; vsize += 3; break;
        case CMD_quad2:        d += 9;  vsize += 6; break;
        case CMD_quad3:        d += 13; vsize += 6; break;
        case CMD_quad2T:       d += 17; vsize += 6; break;
        case CMD_quad3T:       d += 21; vsize += 6; break;
        case CMD_quad2C:       d += 13; vsize += 6; break;
        case CMD_quad3C:       d += 17; vsize += 6; break;
        case CMD_quad2TC:      d += 21; vsize += 6; break;
        case CMD_quad3TC:      d += 25; vsize += 6; break;
        case CMD_quad3TCN:     d += 29; vsize += 6; break;
        case CMD_rectangle:    d += 5;  vsize += 6; break;
        case CMD_rectangleT:   d += 9;  vsize += 6; break;

        default:
          d = dEnd; // stop reading at first invalid cmd
          GX_LOG_ERROR("unknown DrawCmd value: ", cmd);
          break;
      }
    }

    GX_ASSERT(d == dEnd);
    return vsize;
  }

  void setCullFace(int cap)
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

  // Value iterator reading helper functions
  [[nodiscard]] inline int32_t ival(const Value*& ptr) {
    return (ptr++)->ival; }
  [[nodiscard]] inline uint32_t uval(const Value*& ptr) {
    return (ptr++)->uval; }
  [[nodiscard]] inline float fval(const Value*& ptr) {
    return (ptr++)->fval; }
  [[nodiscard]] inline Vec2 fval2(const Value*& ptr) {
    return {fval(ptr), fval(ptr)}; }
  [[nodiscard]] inline Vec3 fval3(const Value*& ptr) {
    return {fval(ptr), fval(ptr), fval(ptr)}; }

  struct Vertex {
    float x, y, z;  // pos
    uint32_t c;     // color (packed 8-bit RGBA)
    float s, t;     // tex coords
    uint32_t n;     // normal (packed 10-bit XYZ)
    uint32_t m;     // mode (ignored for now)
      // TODO: possible values
      //  0-7   color
      //  8-15  transform
      // 16-31  z texture coord
  };

  inline Vertex vertex_val(const Value*& ptr) {
    const float x = fval(ptr);
    const float y = fval(ptr);
    const float z = fval(ptr);
    const float s = fval(ptr);
    const float t = fval(ptr);
    const uint32_t c = uval(ptr);
    const uint32_t n = uval(ptr);
    return {x,y,z,c,s,t,n,0};
  }

  // vertex output functions
  inline void vertex2d(Vertex*& ptr, Vec2 pt, uint32_t c) {
    *ptr++ = {pt.x,pt.y,0.0f, c, 0.0f,0.0f, 0, 0}; }
  inline void vertex2d(Vertex*& ptr, Vec2 pt, uint32_t c, Vec2 tx) {
    *ptr++ = {pt.x,pt.y,0.0f, c, tx.x,tx.y, 0, 0}; }

  inline void vertex3d(Vertex*& ptr, const Vec3& pt, uint32_t c) {
    *ptr++ = {pt.x,pt.y,pt.z, c, 0.0f,0.0f, 0, 0}; }
  inline void vertex3d(
    Vertex*& ptr, const Vec3& pt, uint32_t c, uint32_t n) {
    *ptr++ = {pt.x,pt.y,pt.z, c, 0.0f,0.0f, n, 0}; }
  inline void vertex3d(
    Vertex*& ptr, const Vec3& pt, uint32_t c, Vec2 tx, uint32_t n) {
    *ptr++ = {pt.x,pt.y,pt.z, c, tx.x,tx.y, n, 0}; }

  [[nodiscard]] constexpr Mat4 orthoProjection(int width, int height)
  {
    // simple orthogonal projection to OpenGL screen coordinates
    //  x:[0 width] => x:[-1  1]
    //  y:[0 height]   y:[ 1 -1]
    return {
      2.0f / float(width), 0, 0, 0,
      0, -2.0f / float(height), 0, 0,
        // negative value flips vertical direction for OpenGL
      0, 0, 1, 0,
      -1, 1, 0, 1};
  }

  [[nodiscard]] GLint calcMinFilter(const TextureParams& p)
  {
    // GL_TEXTURE_MIN_FILTER
    if (p.minFilter == FILTER_UNSPECIFIED) { return 0; }

    switch (p.mipFilter) {
      case FILTER_LINEAR:
        return (p.minFilter == FILTER_LINEAR)
          ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR_MIPMAP_NEAREST;
      case FILTER_NEAREST:
        return (p.minFilter == FILTER_LINEAR)
          ? GL_NEAREST_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST;
      default:
        return (p.minFilter == FILTER_LINEAR) ? GL_LINEAR : GL_NEAREST;
    }
  }

  [[nodiscard]] GLint calcMagFilter(const TextureParams& p)
  {
    // GL_TEXTURE_MAG_FILTER
    switch (p.magFilter) {
      case FILTER_LINEAR:  return GL_LINEAR;
      case FILTER_NEAREST: return GL_NEAREST;
      default:             return 0;
    }
  }

  [[nodiscard]] GLint glWrapType(WrapType wt)
  {
    switch (wt) {
      case WRAP_CLAMP_TO_EDGE:        return GL_CLAMP_TO_EDGE;
      case WRAP_CLAMP_TO_BORDER:      return GL_CLAMP_TO_BORDER;
      case WRAP_MIRRORED_REPEAT:      return GL_MIRRORED_REPEAT;
      case WRAP_REPEAT:               return GL_REPEAT;
      case WRAP_MIRROR_CLAMP_TO_EDGE: return GL_MIRROR_CLAMP_TO_EDGE;
      default:                        return 0;
    }
  }
}


// **** OpenGLRenderer ****
namespace gx { template<int VER> class OpenGLRenderer; }

template<int VER>
class gx::OpenGLRenderer final : public gx::Renderer
{
 public:
  // gx::Renderer methods
  bool init(GLFWwindow* win) override;
  bool setSwapInterval(int interval) override;
  bool setFramebufferSize(int width, int height) override;
  TextureHandle newTexture(
    const Image& img, const TextureParams& params) override;
  void freeTexture(TextureID id) override;
  void draw(const DrawList* const* lists, std::size_t count) override;
  void renderFrame(int64_t usecTime) override;

 private:
  static constexpr int SHADER_COUNT = 5;
  GLProgram _sp[SHADER_COUNT];
  GLUniform1i _sp_texUnit[SHADER_COUNT];

  GLBuffer<VER> _uniformBuf;
  struct UniformData {
    // NOTE: for std140 layout, alignment of array types must be 16 bytes
    Mat4 cameraT{INIT_IDENTITY}; // viewT * projT
    Color modColor = WHITE;
    Vec3 lightPos;
    uint32_t pad0 = 0;
    Vec3 lightA;
    uint32_t pad1 = 0;
    Vec3 lightD;
  };

  GLVertexArray<VER> _vao;
  GLBuffer<VER> _vbo;

  struct TextureEntry {
    GLTexture2D<VER> tex;
    int channels = 0;
    int unit = -1;
  };
  std::unordered_map<TextureID,TextureEntry> _textures;

  enum GLOperation : uint32_t {
    OP_null,

    // set uniform data
    OP_cameraT,       // <OP val*16> (17)
    OP_cameraReset,   // <OP> (1)
    OP_modColor,      // <OP rgba8> (2)
    OP_light,         // <OP x y z ambient(rgba8) diffuse(rgba8)> (6)

    // set GL state
    OP_viewport,      // <OP x y w h> (5)
    OP_viewportFull,  // <OP> (1)
    OP_capabilities,  // <OP cap> (2)
    OP_lineWidth,     // <OP width> (2)
    OP_clearColor,    // <OP rgba8> (2)

    // draw
    OP_clear,         // <OP mask> (2)
    OP_drawLines,     // <OP first count> (3)
    OP_drawTriangles, // <OP first count texID> (4)
  };

  Mat4 _orthoT;
  std::vector<Value> _opData;
  GLOperation _lastOp = OP_null;
  int _currentGLCap = -1; // current GL capability state
  std::mutex _glMutex;

  template<class... Args>
  void addOp(GLOperation op, const Args&... args) {
    if constexpr (sizeof...(args) == 0) {
      _opData.push_back(op);
    } else {
      const std::initializer_list<Value> x{op, args...};
      _opData.insert(_opData.end(), x.begin(), x.end());
    }
    _lastOp = op;
  }

  void addOpData(GLOperation op, const Mat4& m) {
    _opData.reserve(_opData.size() + m.size() + 1);
    _opData.push_back(op);
    _opData.insert(_opData.end(), m.begin(), m.end());
  }

  void addOpData(GLOperation op, const Value* begin, const Value* end) {
    _opData.reserve(_opData.size() + std::size_t(end - begin) + 1);
    _opData.push_back(op);
    _opData.insert(_opData.end(), begin, end);
    _lastOp = op;
  }

  void addLine(int32_t& first) {
    if (_lastOp == OP_drawLines) {
      _opData[_opData.size() - 1].ival += 2;
    } else {
      addOp(OP_drawLines, first, 2);
    }
    first += 2;
  }

  void addTriangles(int32_t& first, int32_t count, TextureID tid) {
    if (_lastOp == OP_drawTriangles) {
      const std::size_t s = _opData.size();
      const TextureID last_tid = _opData[s - 1].uval;
      if (last_tid == tid) {
        _opData[s - 2].ival += count;
        first += count;
        return;
      }
    }
    addOp(OP_drawTriangles, first, count, tid);
    first += count;
  }

  void setGLCapabilities(int32_t cap);

  int64_t _lastFrameTime = 0;
  int32_t _frames = 0;
};

template<int VER>
bool OpenGLRenderer<VER>::init(GLFWwindow* win)
{
  GX_ASSERT(win != nullptr);

  const std::lock_guard lg{_glMutex};
  _window = win;

  glfwGetFramebufferSize(win, &_fbWidth, &_fbHeight);
  _orthoT = orthoProjection(_fbWidth, _fbHeight);
  //println("new window size: ", _fbWidth, " x ", _fbHeight);

  _maxTextureSize = GLTexture2D<VER>::maxSize();
  glfwSwapInterval(_swapInterval); // enable V-SYNC

  // NOTES:
  // - matrices in GLSL are column major: P*V*M*v
  // - gx_lib uses row major matrices:    v*M*V*P

  _uniformBuf.init(sizeof(UniformData), nullptr);
  #define UNIFORM_BLOCK_SRC\
    "layout(std140) uniform ub0 {"\
    "  mat4 cameraT;"\
    "  vec4 modColor;"\
    "  vec3 lightPos;"\
    "  vec3 lightA;"\
    "  vec3 lightD;"\
    "};"

  // basic vertex shader
  const GLShader vshader = makeVertexShader<VER>(
    "layout(location = 0) in vec3 in_pos;" // x,y,z
    "layout(location = 1) in uint in_color;"
    "layout(location = 2) in vec2 in_tc;"  // s,t
    UNIFORM_BLOCK_SRC
    "out vec4 v_color;"
    "out vec2 v_texCoord;"
    "void main() {"
    "  v_color = unpackUnorm4x8(in_color) * modColor;"
    "  v_texCoord = in_tc;"
    "  gl_Position = cameraT * vec4(in_pos, 1);"
    "}");

  // vertex shader w/ lighting support
  const GLShader vshader2 = makeVertexShader<VER>(
    "layout(location = 0) in vec3 in_pos;"   // x,y,z
    "layout(location = 1) in uint in_color;"
    "layout(location = 2) in vec2 in_tc;"    // s,t
    "layout(location = 3) in uint in_norm;"  // nx,ny,nz packed 10-bits each
    UNIFORM_BLOCK_SRC
    "out vec3 v_pos;"
    "out vec3 v_norm;"
    "out vec4 v_color;"
    "out vec2 v_texCoord;"
    "out vec3 v_lightPos;"
    "out vec3 v_lightA;"
    "out vec3 v_lightD;"

    "vec3 unpackNormal(uint n) {"
    "  float x = float(int(n & 0x3ffU) - 511) / 511.0;"
    "  float y = float(int((n>>10) & 0x3ffU) - 511) / 511.0;"
    "  float z = float(int((n>>20) & 0x3ffU) - 511) / 511.0;"
    "  return vec3(x, y, z);"
    "}"

    "void main() {"
    "  v_pos = in_pos;"
    "  v_norm = unpackNormal(in_norm);"
    "  v_color = unpackUnorm4x8(in_color) * modColor;"
    "  v_texCoord = in_tc;"
    "  v_lightPos = lightPos;"
    "  v_lightA = lightA;"
    "  v_lightD = lightD;"
    "  gl_Position = cameraT * vec4(in_pos, 1);"
    "}");

  // solid color shader
  _sp[0] = makeProgram<VER>(vshader, makeFragmentShader<VER>(
    "in vec4 v_color;"
    "out vec4 fragColor;"
    "void main() { fragColor = v_color; }"));

  // mono color texture shader (fonts)
  _sp[1] = makeProgram<VER>(vshader, makeFragmentShader<VER>(
    "in vec2 v_texCoord;"
    "in vec4 v_color;"
    "uniform sampler2D texUnit;"
    "out vec4 fragColor;"
    "void main() {"
    "  float a = texture(texUnit, v_texCoord).r;"
    "  if (a == 0.0) discard;"
    "  fragColor = vec4(v_color.rgb, v_color.a * a);"
    "}"));

  // full color texture shader (images)
  _sp[2] = makeProgram<VER>(vshader, makeFragmentShader<VER>(
    "in vec2 v_texCoord;"
    "in vec4 v_color;"
    "uniform sampler2D texUnit;"
    "out vec4 fragColor;"
    "void main() { fragColor = texture(texUnit, v_texCoord) * v_color; }"));

  // 3d shader w/ lighting
  _sp[3] = makeProgram<VER>(vshader2, makeFragmentShader<VER>(
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
    "}"));

  // textured 3d shader w/ lighting
  _sp[4] = makeProgram<VER>(vshader2, makeFragmentShader<VER>(
    "in vec3 v_pos;"
    "in vec3 v_norm;"
    "in vec4 v_color;"
    "in vec2 v_texCoord;"
    "in vec3 v_lightPos;"
    "in vec3 v_lightA;"
    "in vec3 v_lightD;"
    "uniform sampler2D texUnit;"
    "out vec4 fragColor;"
    "void main() {"
    "  vec3 lightDir = normalize(v_lightPos - v_pos);"
    "  float lt = max(dot(normalize(v_norm), lightDir), 0.0);"
    "  fragColor = texture(texUnit, v_texCoord) * v_color * vec4((v_lightD * lt) + v_lightA, 1.0);"
    "}"));

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
  GX_GLCALL(glGetFloatv, GL_ALIASED_LINE_WIDTH_RANGE, val);
  println("GL_ALIASED_LINE_WIDTH_RANGE: ", val[0], " ", val[1]);

  GX_GLCALL(glGetFloatv, GL_SMOOTH_LINE_WIDTH_RANGE, val);
  println("GL_SMOOTH_LINE_WIDTH_RANGE: ", val[0], " ", val[1]);

  GX_GLCALL(glGetFloatv, GL_SMOOTH_LINE_WIDTH_GRANULARITY, val);
  println("GL_SMOOTH_LINE_WIDTH_GRANULARITY: ", val[0]);
#endif

  return status;
}

template<int VER>
bool OpenGLRenderer<VER>::setSwapInterval(int interval)
{
  const std::lock_guard lg{_glMutex};
  _swapInterval = std::clamp(interval, 0, 60);
  glfwSwapInterval(_swapInterval);
  return true;
}

template<int VER>
bool OpenGLRenderer<VER>::setFramebufferSize(int width, int height)
{
  const std::lock_guard lg{_glMutex};
  _fbWidth = width;
  _fbHeight = height;
  _orthoT = orthoProjection(_fbWidth, _fbHeight);
  return true;
}

template<int VER>
TextureHandle OpenGLRenderer<VER>::newTexture(
  const Image& img, const TextureParams& params)
{
  GLenum texformat, imgformat;
  switch (img.channels()) {
    case 1: texformat = GL_R8;    imgformat = GL_RED;  break;
    case 2: texformat = GL_RG8;   imgformat = GL_RG;   break;
    case 3: texformat = GL_RGB8;  imgformat = GL_RGB;  break;
    case 4: texformat = GL_RGBA8; imgformat = GL_RGBA; break;
    default: return {};
  }

  const TextureID id = newTextureID();

  const std::lock_guard lg{_glMutex};
  TextureEntry& te = _textures[id];

  setCurrentContext(_window);

  auto& t = te.tex;
  t.init(std::max(1, params.levels), texformat, img.width(), img.height());
  te.channels = img.channels();

  t.setSubImage(
    0, 0, 0, img.width(), img.height(), imgformat, img.data());
  if (params.levels > 1) { t.generateMipmap(); }

  {
    const GLint val = calcMinFilter(params);
    if (val != 0) {
      t.setParameter(GL_TEXTURE_MIN_FILTER, val);
    }
  }

  if (params.magFilter != FILTER_UNSPECIFIED) {
    t.setParameter(GL_TEXTURE_MAG_FILTER, calcMagFilter(params));
  }

  if (params.wrapS != WRAP_UNSPECIFIED) {
    t.setParameter(GL_TEXTURE_WRAP_S, glWrapType(params.wrapS));
  }

  if (params.wrapT != WRAP_UNSPECIFIED) {
    t.setParameter(GL_TEXTURE_WRAP_T, glWrapType(params.wrapT));
  }

  return TextureHandle{id};
}

template<int VER>
void OpenGLRenderer<VER>::freeTexture(TextureID id)
{
  const std::lock_guard lg{_glMutex};
  setCurrentContext(_window);
  _textures.erase(id);
}

template<int VER>
void OpenGLRenderer<VER>::draw(const DrawList* const* lists, std::size_t count)
{
  GX_ASSERT(lists != nullptr);
  const DrawList* const* listsEnd = lists + count;

  std::size_t vsize = 0; // vertices needed for all layers
  for (const DrawList* const* dlPtr = lists; dlPtr != listsEnd; ++dlPtr) {
    GX_ASSERT(*dlPtr != nullptr);
    vsize += calcVSize(**dlPtr);
  }

  const std::lock_guard lg{_glMutex};
  setCurrentContext(_window);

  _opData.clear();
  _lastOp = OP_null;

  Vertex* ptr = nullptr;
  if (vsize == 0) {
    _vbo = {};
    _vao = {};
  } else {
    if (!_vbo) { _vbo.init(); }
    _vbo.setData(GLsizei(vsize * sizeof(Vertex)), nullptr, GL_STREAM_DRAW);
    ptr = static_cast<Vertex*>(_vbo.map(GL_WRITE_ONLY));
    GX_ASSERT(ptr != nullptr);
  }

  // general triangle layout
  //  0--1
  //  | /|
  //  |/ |
  //  2--3

  bool cameraSet = false;
  int32_t first = 0;
  int32_t cap = -1;

  for (const DrawList* const* dlPtr = lists; dlPtr != listsEnd; ++dlPtr) {
    const DrawList& dl = **dlPtr;
    uint32_t color = 0;
    TextureID tid = 0;
    uint32_t normal = 0;
    Vec3 linePt;
    uint32_t lineColor = 0;

    const Value* data     = dl.data();
    const Value* data_end = data + dl.size();
    for (const Value* d = data; d != data_end; ) {
      const uint32_t cmd = (d++)->uval;
      switch (cmd) {
        case CMD_viewport: {
          const Value* d0 = d; d += 4;
          addOpData(OP_viewport, d0, d);
          break;
        }
        case CMD_viewportFull:
          addOp(OP_viewportFull);
          break;

        case CMD_color:   color  = uval(d); break;
        case CMD_texture: tid    = uval(d); break;
        case CMD_normal:  normal = uval(d); break;

        case CMD_modColor:
          addOp(OP_modColor, *d++); break;

        case CMD_capabilities: {
          const int32_t newCap = ival(d);
          if (cap != newCap) {
            cap = newCap;
            addOp(OP_capabilities, cap);
          }
          break;
        }

        case CMD_camera: {
          Mat4 viewT{INIT_NONE}, projT{INIT_NONE};
          std::memcpy(viewT.data(), d, sizeof(float)*16); d += 16;
          std::memcpy(projT.data(), d, sizeof(float)*16); d += 16;
          addOpData(OP_cameraT, viewT * projT);
          cameraSet = true;
          break;
        }
        case CMD_cameraReset:
          if (cameraSet) {
            addOp(OP_cameraReset);
            cameraSet = false;
          }
          break;

        case CMD_light: {
          const Value* d0 = d; d += 5;
          addOpData(OP_light, d0, d); // pos x/y/z, ambient, diffuse
          break;
        }

        case CMD_lineWidth:
          addOp(OP_lineWidth, *d++);
          break;

        case CMD_clearView:
          addOp(OP_clearColor, *d++);
          addOp(OP_clear, uint32_t(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
          break;

        case CMD_line2: {
          vertex2d(ptr, fval2(d), color);
          vertex2d(ptr, fval2(d), color);
          addLine(first);
          break;
        }
        case CMD_line3: {
          vertex3d(ptr, fval3(d), color);
          vertex3d(ptr, fval3(d), color);
          addLine(first);
          break;
        }
        case CMD_line2C: {
          const Vec2 p0 = fval2(d); const uint32_t c0 = uval(d);
          const Vec2 p1 = fval2(d); const uint32_t c1 = uval(d);
          vertex2d(ptr, p0, c0);
          vertex2d(ptr, p1, c1);
          addLine(first);
          break;
        }
        case CMD_line3C: {
          const Vec3 p0 = fval3(d); const uint32_t c0 = uval(d);
          const Vec3 p1 = fval3(d); const uint32_t c1 = uval(d);
          vertex3d(ptr, p0, c0);
          vertex3d(ptr, p1, c1);
          addLine(first);
          break;
        }
        case CMD_lineStart2:
          linePt.set(fval2(d), 0); lineColor = color; break;
        case CMD_lineTo2: {
          vertex3d(ptr, linePt, lineColor);
          linePt.set(fval2(d), 0); lineColor = color;
          vertex3d(ptr, linePt, lineColor);
          addLine(first);
          break;
        }
        case CMD_lineStart3:
          linePt = fval3(d); lineColor = color; break;
        case CMD_lineTo3: {
          vertex3d(ptr, linePt, lineColor);
          linePt = fval3(d); lineColor = color;
          vertex3d(ptr, linePt, lineColor);
          addLine(first);
          break;
        }
        case CMD_lineStart2C:
          linePt.set(fval2(d), 0); lineColor = uval(d); break;
        case CMD_lineTo2C: {
          vertex3d(ptr, linePt, lineColor);
          linePt.set(fval2(d), 0); lineColor = uval(d);
          vertex3d(ptr, linePt, lineColor);
          addLine(first);
          break;
        }
        case CMD_lineStart3C:
          linePt = fval3(d); lineColor = uval(d); break;
        case CMD_lineTo3C: {
          vertex3d(ptr, linePt, lineColor);
          linePt = fval3(d); lineColor = uval(d);
          vertex3d(ptr, linePt, lineColor);
          addLine(first);
          break;
        }
        case CMD_triangle2: {
          vertex2d(ptr, fval2(d), color);
          vertex2d(ptr, fval2(d), color);
          vertex2d(ptr, fval2(d), color);
          addTriangles(first, 3, 0);
          break;
        }
        case CMD_triangle3: {
          vertex3d(ptr, fval3(d), color, normal);
          vertex3d(ptr, fval3(d), color, normal);
          vertex3d(ptr, fval3(d), color, normal);
          addTriangles(first, 3, 0);
          break;
        }
        case CMD_triangle2T: {
          const Vec2 p0 = fval2(d), t0 = fval2(d);
          const Vec2 p1 = fval2(d), t1 = fval2(d);
          const Vec2 p2 = fval2(d), t2 = fval2(d);
          vertex2d(ptr, p0, color, t0);
          vertex2d(ptr, p1, color, t1);
          vertex2d(ptr, p2, color, t2);
          addTriangles(first, 3, tid);
          break;
        }
        case CMD_triangle3T: {
          const Vec3 p0 = fval3(d); const Vec2 t0 = fval2(d);
          const Vec3 p1 = fval3(d); const Vec2 t1 = fval2(d);
          const Vec3 p2 = fval3(d); const Vec2 t2 = fval2(d);
          vertex3d(ptr, p0, color, t0, normal);
          vertex3d(ptr, p1, color, t1, normal);
          vertex3d(ptr, p2, color, t2, normal);
          addTriangles(first, 3, tid);
          break;
        }
        case CMD_triangle2C: {
          const Vec2 p0 = fval2(d); const uint32_t c0 = uval(d);
          const Vec2 p1 = fval2(d); const uint32_t c1 = uval(d);
          const Vec2 p2 = fval2(d); const uint32_t c2 = uval(d);
          vertex2d(ptr, p0, c0);
          vertex2d(ptr, p1, c1);
          vertex2d(ptr, p2, c2);
          addTriangles(first, 3, 0);
          break;
        }
        case CMD_triangle3C: {
          const Vec3 p0 = fval3(d); const uint32_t c0 = uval(d);
          const Vec3 p1 = fval3(d); const uint32_t c1 = uval(d);
          const Vec3 p2 = fval3(d); const uint32_t c2 = uval(d);
          vertex3d(ptr, p0, c0, normal);
          vertex3d(ptr, p1, c1, normal);
          vertex3d(ptr, p2, c2, normal);
          addTriangles(first, 3, 0);
          break;
        }
        case CMD_triangle2TC: {
          const Vec2 p0 = fval2(d), t0 = fval2(d); const uint32_t c0 = uval(d);
          const Vec2 p1 = fval2(d), t1 = fval2(d); const uint32_t c1 = uval(d);
          const Vec2 p2 = fval2(d), t2 = fval2(d); const uint32_t c2 = uval(d);
          vertex2d(ptr, p0, c0, t0);
          vertex2d(ptr, p1, c1, t1);
          vertex2d(ptr, p2, c2, t2);
          addTriangles(first, 3, tid);
          break;
        }
        case CMD_triangle3TC: {
          const Vec3 p0 = fval3(d); const Vec2 t0 = fval2(d);
          const uint32_t c0 = uval(d);
          const Vec3 p1 = fval3(d); const Vec2 t1 = fval2(d);
          const uint32_t c1 = uval(d);
          const Vec3 p2 = fval3(d); const Vec2 t2 = fval2(d);
          const uint32_t c2 = uval(d);
          vertex3d(ptr, p0, c0, t0, normal);
          vertex3d(ptr, p1, c1, t1, normal);
          vertex3d(ptr, p2, c2, t2, normal);
          addTriangles(first, 3, tid);
          break;
        }
        case CMD_triangle3TCN: {
          *ptr++ = vertex_val(d);
          *ptr++ = vertex_val(d);
          *ptr++ = vertex_val(d);
          addTriangles(first, 3, tid);
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
          addTriangles(first, 6, 0);
          break;
        }
        case CMD_quad3: {
          const Vec3 p0 = fval3(d), p1 = fval3(d);
          const Vec3 p2 = fval3(d), p3 = fval3(d);
          vertex3d(ptr, p0, color, normal);
          vertex3d(ptr, p1, color, normal);
          vertex3d(ptr, p2, color, normal);
          vertex3d(ptr, p1, color, normal);
          vertex3d(ptr, p3, color, normal);
          vertex3d(ptr, p2, color, normal);
          addTriangles(first, 6, 0);
          break;
        }
        case CMD_quad2T: {
          const Vec2 p0 = fval2(d), t0 = fval2(d);
          const Vec2 p1 = fval2(d), t1 = fval2(d);
          const Vec2 p2 = fval2(d), t2 = fval2(d);
          const Vec2 p3 = fval2(d), t3 = fval2(d);
          vertex2d(ptr, p0, color, t0);
          vertex2d(ptr, p1, color, t1);
          vertex2d(ptr, p2, color, t2);
          vertex2d(ptr, p1, color, t1);
          vertex2d(ptr, p3, color, t3);
          vertex2d(ptr, p2, color, t2);
          addTriangles(first, 6, tid);
          break;
        }
        case CMD_quad3T: {
          const Vec3 p0 = fval3(d); const Vec2 t0 = fval2(d);
          const Vec3 p1 = fval3(d); const Vec2 t1 = fval2(d);
          const Vec3 p2 = fval3(d); const Vec2 t2 = fval2(d);
          const Vec3 p3 = fval3(d); const Vec2 t3 = fval2(d);
          vertex3d(ptr, p0, color, t0, normal);
          vertex3d(ptr, p1, color, t1, normal);
          vertex3d(ptr, p2, color, t2, normal);
          vertex3d(ptr, p1, color, t1, normal);
          vertex3d(ptr, p3, color, t3, normal);
          vertex3d(ptr, p2, color, t2, normal);
          addTriangles(first, 6, tid);
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
          addTriangles(first, 6, 0);
          break;
        }
        case CMD_quad3C: {
          const Vec3 p0 = fval3(d); const uint32_t c0 = uval(d);
          const Vec3 p1 = fval3(d); const uint32_t c1 = uval(d);
          const Vec3 p2 = fval3(d); const uint32_t c2 = uval(d);
          const Vec3 p3 = fval3(d); const uint32_t c3 = uval(d);
          vertex3d(ptr, p0, c0, normal);
          vertex3d(ptr, p1, c1, normal);
          vertex3d(ptr, p2, c2, normal);
          vertex3d(ptr, p1, c1, normal);
          vertex3d(ptr, p3, c3, normal);
          vertex3d(ptr, p2, c2, normal);
          addTriangles(first, 6, 0);
          break;
        }
        case CMD_quad2TC: {
          const Vec2 p0 = fval2(d), t0 = fval2(d); const uint32_t c0 = uval(d);
          const Vec2 p1 = fval2(d), t1 = fval2(d); const uint32_t c1 = uval(d);
          const Vec2 p2 = fval2(d), t2 = fval2(d); const uint32_t c2 = uval(d);
          const Vec2 p3 = fval2(d), t3 = fval2(d); const uint32_t c3 = uval(d);
          vertex2d(ptr, p0, c0, t0);
          vertex2d(ptr, p1, c1, t1);
          vertex2d(ptr, p2, c2, t2);
          vertex2d(ptr, p1, c1, t1);
          vertex2d(ptr, p3, c3, t3);
          vertex2d(ptr, p2, c2, t2);
          addTriangles(first, 6, tid);
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
          vertex3d(ptr, p0, c0, t0, normal);
          vertex3d(ptr, p1, c1, t1, normal);
          vertex3d(ptr, p2, c2, t2, normal);
          vertex3d(ptr, p1, c1, t1, normal);
          vertex3d(ptr, p3, c3, t3, normal);
          vertex3d(ptr, p2, c2, t2, normal);
          addTriangles(first, 6, tid);
          break;
        }
        case CMD_quad3TCN: {
          const Vertex v0 = vertex_val(d);
          const Vertex v1 = vertex_val(d);
          const Vertex v2 = vertex_val(d);
          const Vertex v3 = vertex_val(d);
          *ptr++ = v0;
          *ptr++ = v1;
          *ptr++ = v2;
          *ptr++ = v1;
          *ptr++ = v3;
          *ptr++ = v2;
          addTriangles(first, 6, tid);
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
          addTriangles(first, 6, 0);
          break;
        }
        case CMD_rectangleT: {
          const Vec2 p0 = fval2(d), t0 = fval2(d);
          const Vec2 p3 = fval2(d), t3 = fval2(d);
          const Vec2 p1{p3.x,p0.y}, t1{t3.x,t0.y};
          const Vec2 p2{p0.x,p3.y}, t2{t0.x,t3.y};
          vertex2d(ptr, p0, color, t0);
          vertex2d(ptr, p1, color, t1);
          vertex2d(ptr, p2, color, t2);
          vertex2d(ptr, p1, color, t1);
          vertex2d(ptr, p3, color, t3);
          vertex2d(ptr, p2, color, t2);
          addTriangles(first, 6, tid);
          break;
        }
        default:
          d = data_end; // stop processing at first invalid cmd
          break;
      }
    }
  }

  if (_vbo) {
    _vbo.unmap();
    if (!_vao) {
      _vao.init();
      static_assert(sizeof(Vertex) == 32);

      _vao.enableAttrib(0); // vec3 (x,y,z)
      _vao.setAttrib(0, _vbo, 0, sizeof(Vertex), 3, GL_FLOAT, GL_FALSE);

      _vao.enableAttrib(1); // uint (r,g,b,a 8:8:8:8 packed int)
      _vao.setAttribI(1, _vbo, 12, sizeof(Vertex), 1, GL_UNSIGNED_INT);

      _vao.enableAttrib(2); // vec2 (s,t)
      _vao.setAttrib(2, _vbo, 16, sizeof(Vertex), 2, GL_FLOAT, GL_FALSE);

      _vao.enableAttrib(3); // uint (x,y,z 10:10:10 packed int)
      _vao.setAttribI(3, _vbo, 24, sizeof(Vertex), 1, GL_UNSIGNED_INT);

      _vao.enableAttrib(4); // uint
      _vao.setAttribI(4, _vbo, 28, sizeof(Vertex), 1, GL_UNSIGNED_INT);
    }
  }

#if 0
  std::size_t dsize = 0;
  for (const DrawList* const* dlPtr = lists; dlPtr != listsEnd; ++dlPtr) {
    dsize += (*dlPtr)->size();
  }
  println_err("entries:", dsize, "  vertices:", vsize,
              "  opData:", _opData.size());
#endif
}

template<int VER>
void OpenGLRenderer<VER>::renderFrame(int64_t usecTime)
{
  const std::lock_guard lg{_glMutex};
  if (_opData.empty()) { return; }

  setCurrentContext(_window);

  // set default GL state
  GX_GLCALL(glViewport, 0, 0, _fbWidth, _fbHeight);
  GX_GLCALL(glClearDepth, 1.0);
  GX_GLCALL(glDepthMask, GL_TRUE);
  GX_GLCALL(glDepthFunc, GL_LESS);
  GX_GLCALL(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  GX_GLCALL(glLineWidth, 1.0f);
  GX_GLCALL(glEnable, GL_LINE_SMOOTH);
  GX_GLCALL(glFrontFace, GL_CW);

  // clear texture unit assignments
  for (auto& t : _textures) { t.second.unit = -1; }

  _currentGLCap = -1; // force all capabilities to be set initially
  _uniformBuf.bindBase(GL_UNIFORM_BUFFER, 0);

  bool udChanged = true;
  UniformData ud{};
  ud.cameraT = _orthoT; // default 2D projection

  // draw
  _vao.bind();
  int lastShader = -1;
  int nextTexUnit = 0;
  int texUnit = -1;
  int32_t newGLCap = BLEND; // default GL capabilities
  bool useLight = false;

  const Value* data     = _opData.data();
  const Value* data_end = data + _opData.size();
  for (const Value* d = data; d < data_end; ) {
    const uint32_t op = (d++)->uval;
    switch (op) {
      case OP_cameraT:
        std::memcpy(ud.cameraT.data(), d, sizeof(float)*16); d += 16;
        udChanged = true;
        break;
      case OP_cameraReset:
        ud.cameraT = _orthoT;
        udChanged = true;
        break;
      case OP_modColor:
        ud.modColor = unpackRGBA8((d++)->uval);
        udChanged = true;
        break;
      case OP_light:
        std::memcpy(ud.lightPos.data(), d, sizeof(float)*3); d += 3;
        ud.lightA = unpackRGBA8((d++)->uval).rgb();
        ud.lightD = unpackRGBA8((d++)->uval).rgb();
        udChanged = true;
        break;
      case OP_lineWidth:
        GX_GLCALL(glLineWidth, (d++)->fval);
        break;
      case OP_clearColor: {
        const Color c = unpackRGBA8((d++)->uval);
        GX_GLCALL(glClearColor, c.r, c.g, c.b, c.a);
        break;
      }
      case OP_viewport: {
        const int32_t x = (d++)->ival;
        const int32_t y = (d++)->ival;
        const int32_t w = (d++)->ival;
        const int32_t h = (d++)->ival;
        GX_GLCALL(glViewport, x, _fbHeight - y - h, w, h);
          // change upperLeft origin to lowerLeft for OpenGL
        break;
      }
      case OP_viewportFull:
        GX_GLCALL(glViewport, 0, 0, _fbWidth, _fbHeight);
        break;
      case OP_capabilities: {
        const int32_t cap = (d++)->ival;
        newGLCap = cap & ~LIGHTING;
        useLight = cap & LIGHTING;
        break;
      }
      case OP_clear:
        GX_GLCALL(glClear, (d++)->uval);
        break;
      case OP_drawLines: {
        const GLint first = (d++)->ival;
        const GLsizei count = (d++)->ival;
        if (_currentGLCap != newGLCap) { setGLCapabilities(newGLCap); }
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
      case OP_drawTriangles: {
        const GLint first = (d++)->ival;
        const GLsizei count = (d++)->ival;
        const TextureID tid = (d++)->uval;
        if (_currentGLCap != newGLCap) { setGLCapabilities(newGLCap); }
        if (udChanged) {
          _uniformBuf.setSubData(0, sizeof(ud), &ud);
          udChanged = false;
        }

        bool setUnit = false;

        // shader values
        //  0 - flat shader
        //  1 - mono texture shader
        //  2 - color texture shader
        //  3 - lit flat shader
        //  4 - lit texture shader
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
            if (useLight) {
              shader = 4;
            } else {
              shader = (entry.channels == 1) ? 1 : 2;
            }
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
  GX_CHECK_GL_ERRORS("GL error");
  GLClearState();

  // frame rate calculation
  ++_frames;
  const int64_t t = secTime();
  if (t - _lastFrameTime >= 1) {
    _frameRate = _frames;
    _frames = 0;
    _lastFrameTime = t;
    //println("frame rate: ", _frameRate);
  }
}

template<int VER>
void OpenGLRenderer<VER>::setGLCapabilities(int32_t cap)
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


// **** Functions ****
std::unique_ptr<Renderer> gx::makeOpenGLRenderer(GLFWwindow* win)
{
  setCurrentContext(win);
  if (!GLSetupContext(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress))) {
    return {};
  }

  GX_LOG_INFO("GLAD generator version: ", GLAD_GENERATOR_VERSION);
  GX_LOG_INFO("GL_VENDOR: ", getGLString(GL_VENDOR));
  GX_LOG_INFO("GL_RENDERER: ", getGLString(GL_RENDERER));
  GX_LOG_INFO("GL_VERSION: ", getGLString(GL_VERSION));
  GX_LOG_INFO("GL_SHADING_LANGUAGE_VERSION: ", getGLString(GL_SHADING_LANGUAGE_VERSION));

  const int major_ver = GLAD_VERSION_MAJOR(GLVersion);
  const int minor_ver = GLAD_VERSION_MINOR(GLVersion);
  const int ver = (major_ver * 10) + minor_ver;
  if (ver < 33) {
    GX_LOG_ERROR("OpenGL 3.3 or higher is required");
    return {};
  }

  std::unique_ptr<Renderer> ren;
  if (ver >= 45) {
    GX_LOG_INFO("OpenGL 4.5 GX_LIB Renderer");
    ren = std::make_unique<OpenGLRenderer<45>>();
  } else if (ver >= 43) {
    GX_LOG_INFO("OpenGL 4.3 GX_LIB Renderer");
    ren = std::make_unique<OpenGLRenderer<43>>();
  } else if (ver >= 42) {
    GX_LOG_INFO("OpenGL 4.2 GX_LIB Renderer");
    ren = std::make_unique<OpenGLRenderer<42>>();
  } else {
    GX_LOG_INFO("OpenGL 3.3 GX_LIB Renderer");
    if (GLAD_GL_ARB_shading_language_packing == 0) {
      GX_LOG_ERROR("GL_ARB_shading_language_packing required");
      return {};
    }

    ren = std::make_unique<OpenGLRenderer<33>>();
  }

  if (!ren->init(win)) {
    GX_LOG_ERROR("OpenGLRenderer::init() failed");
    return {};
  }

  return ren;
}
