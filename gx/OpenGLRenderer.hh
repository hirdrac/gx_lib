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
            std::initializer_list<DrawLayer*> dl) override;
  void renderFrame() override;

 private:
  int _width = 0, _height = 0;

  static constexpr int SHADER_COUNT = 4;
  GLProgram _sp[SHADER_COUNT];
  GLUniform1i _sp_texUnit[SHADER_COUNT];

  GLBuffer _uniformBuf;
  struct UniformData {
    Mat4 viewT;
    Mat4 projT;
    Vec3 lightPos;
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

  struct DrawCall {
    GLsizei count;
    GLenum mode; // GL_LINES, GL_TRIANGLES
    TextureID texID;
    float lineWidth;
    const DrawLayer* layerPtr = nullptr;
  };
  std::vector<DrawCall> _drawCalls;
  int _currentGLCap = -1; // current GL capability state
  std::mutex _glMutex;

  void addDrawCall(GLsizei count, GLenum mode, TextureID texID,
                   float lineWidth, const DrawLayer* layerPtr) {
    if (!_drawCalls.empty()) {
      DrawCall& dc = _drawCalls.back();
      if (mode == dc.mode && texID == dc.texID && lineWidth == dc.lineWidth
          && layerPtr == dc.layerPtr) {
	dc.count += count;
	return;
      }
    }
    _drawCalls.push_back({count, mode, texID, lineWidth, layerPtr});
  }

  void setGLCapabilities(int cap);
  static void setCullFace(int cap);
};
