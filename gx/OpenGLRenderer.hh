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
  void freeTexture(TextureID id) override { _textures.erase(id); }
  void renderFrame() override;

 private:
  GLProgram _sp[3];
  GLUniform1i _sp_texUnit[3];

  GLBuffer _uniformBuf;
  struct UniformData {
    Mat4 viewT;
    Mat4 projT;
    uint32_t modColor;
  };

  GLVertexArray _vao;
  GLBuffer _vbo;

  struct TextureEntry {
    GLTexture2D tex;
    int flags = 0;
    int unit = -1;
    int shader = 0; // 1-mono, 2-color
  };
  std::unordered_map<TextureID,TextureEntry> _textures;

  struct DrawCall {
    GLsizei count;
    GLenum mode; // GL_LINES, GL_TRIANGLES
    uint32_t modColor;
    TextureID texID;
    float lineWidth;
    int transformID;
    int capabilities; // Renderer::CapabilityEnum bitfield
  };
  std::vector<DrawCall> _drawCalls;
  int _currentGLCap = -1; // current GL capability state

  void addDrawCall(GLsizei count, GLenum mode, uint32_t modColor,
                   TextureID texID, float lineWidth, int transID, int cap) {
    if (!_drawCalls.empty()) {
      DrawCall& dc = _drawCalls.back();
      if (mode == dc.mode && modColor == dc.modColor && texID == dc.texID
          && lineWidth == dc.lineWidth && transID == dc.transformID
          && cap == dc.capabilities) {
	dc.count += count;
	return;
      }
    }
    _drawCalls.push_back(
      {count, mode, modColor, texID, lineWidth, transID, cap});
  }

  void setGLCapabilities(int cap);
  static void setCullFace(int cap);
  void setupBuffer();
};
