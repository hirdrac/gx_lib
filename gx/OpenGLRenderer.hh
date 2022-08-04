//
// gx/OpenGLRenderer.hh
// Copyright (C) 2022 Richard Bradley
//

#pragma once
#include "Renderer.hh"
#include "GLProgram.hh"
#include "GLBuffer.hh"
#include "GLVertexArray.hh"
#include "GLUniform.hh"
#include "GLTexture.hh"
#include <vector>
#include <unordered_map>
#include <mutex>


namespace gx {
  class OpenGLRenderer;
}

class gx::OpenGLRenderer final : public gx::Renderer
{
 public:
  // gx::Renderer methods
  void setWindowHints(bool debug) override;
  bool init(GLFWwindow* win) override;

  TextureID setTexture(TextureID id, const Image& img, int levels,
                       FilterType minFilter, FilterType magFilter) override;
  void freeTexture(TextureID id) override;
  void draw(int width, int height,
            std::initializer_list<const DrawLayer*> dl) override;
  void renderFrame() override;

 private:
  int _width = 0, _height = 0;

  static constexpr int SHADER_COUNT = 4;
  GLProgram _sp[SHADER_COUNT];
  GLUniform1i _sp_texUnit[SHADER_COUNT];

  GLBuffer _uniformBuf;
  struct UniformData {
    Mat4 viewT{INIT_ZERO};
    Mat4 projT{INIT_ZERO};
    Vec3 lightPos{INIT_ZERO};
    uint32_t lightA;
    uint32_t lightD;
    uint32_t modColor;
  };

  GLVertexArray _vao;
  GLBuffer _vbo;

  struct TextureEntry {
    GLTexture2D tex;
    int flags = 0;
    int unit = -1;
    int channels = 0;
  };
  std::unordered_map<TextureID,TextureEntry> _textures;

  enum GLOperation : uint32_t {
    OP_null,

    // set uniform data
    OP_viewT,         // <OP val*16> (17)
    OP_projT,         // <OP val*16> (17)
    OP_modColor,      // <OP rgba8> (2)
    OP_light,         // <OP x y z ambient(rgba8) diffuse(rgba8)> (6)
    OP_no_light,      // <OP> (1)

    // set GL state
    OP_capabilities,  // <OP cap> (2)
    OP_lineWidth,     // <OP width> (2)
    OP_bgColor,       // <OP rgba8> (2)

    // draw
    OP_clear_color,    // <OP> (1)
    OP_clear_depth,    // <OP> (1)
    OP_clear_all,      // <OP> (1)
    OP_draw_lines,     // <OP first count> (3)
    OP_draw_triangles, // <OP first count texID> (4)
  };

  struct OpEntry {
    union {
      GLOperation op;
      float    fval;
      int32_t  ival;
      uint32_t uval;
    };

    OpEntry(GLOperation o) : op{o} { }
    OpEntry(float f) : fval{f} { }
    OpEntry(int32_t i) : ival{i} { }
    OpEntry(uint32_t u) : uval{u} { }
  };
  static_assert(sizeof(OpEntry) == 4);

  std::vector<OpEntry> _opData;
  GLOperation _lastOp = OP_null;
  int _currentGLCap = -1; // current GL capability state
  std::mutex _glMutex;

  void addOp(GLOperation op, const Mat4& m) {
    _opData.reserve(_opData.size() + 17);
    _opData.push_back(op);
    for (float v : m) { _opData.push_back(v); }
    _lastOp = op;
  }

  template<class... Args>
  void addOp(GLOperation op, const Args&... args) {
    _opData.reserve(_opData.size() + sizeof...(args) + 1);
    _opData.push_back(op);
    (_opData.push_back(args), ...);
    _lastOp = op;
  }

  void addDrawLines(int32_t& first, int32_t count) {
    if (_lastOp == OP_draw_lines) {
      const std::size_t s = _opData.size();
      const int32_t last_first = _opData[s - 2].ival;
      int32_t& last_count = _opData[s - 1].ival;
      if (first == (last_first + last_count)) {
        last_count += count;
        first += count;
        return;
      }
    }
    addOp(OP_draw_lines, first, count);
    first += count;
  }

  void addDrawTriangles(int32_t& first, int32_t count, TextureID tid) {
    if (_lastOp == OP_draw_triangles) {
      const std::size_t s = _opData.size();
      const int32_t last_first = _opData[s - 3].ival;
      int32_t& last_count = _opData[s - 2].ival;
      const TextureID last_tid = _opData[s - 1].uval;
      if (first == (last_first + last_count) && last_tid == tid) {
        last_count += count;
        first += count;
        return;
      }
    }
    addOp(OP_draw_triangles, first, count, tid);
    first += count;
  }

  void setGLCapabilities(int cap);
  static void setCullFace(int cap);
};
