//
// gx/OpenGLRenderer.hh
// Copyright (C) 2021 Richard Bradley
//

// FIXME - make TextureID unique across all Renderer instances

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
  OpenGLRenderer() = default;

  // gx::Renderer methods
  void setWindowHints(bool debug) override;
  bool init(GLFWwindow* win) override;

  TextureID setTexture(TextureID id, const Image& img, int levels,
                       FilterType minFilter, FilterType magFilter) override;
  void freeTexture(TextureID id) override { _textures.erase(id); }
  void setBGColor(float r, float g, float b) override { _bgColor.set(r,g,b); }

  void clearFrame(int width, int height) override;
  void renderFrame() override;

 private:
  int _width = 0, _height = 0;
  Vec3 _bgColor = {0,0,0};

  GLProgram _sp[3];
  GLUniformMat4f _sp_trans[3];
  GLUniform1ui _sp_modColor[3];
  GLUniform1i _sp_texUnit[3];

  GLVertexArray _vao;
  GLBuffer _vbo;

  struct TextureEntry {
    GLTexture2D tex;
    int flags = 0;
    int unit = -1;
    int shader = 0; // 1-mono, 2-color
  };
  std::unordered_map<TextureID,TextureEntry> _textures;
  TextureID _lastTexID = 0;

  struct TransformEntry {
    Mat4 view, projection;
  };
  std::vector<TransformEntry> _transforms;

  struct DrawCall {
    GLsizei count;
    GLenum mode; // GL_LINES, GL_TRIANGLES
    uint32_t modColor;
    TextureID texID;
    float lineWidth;
    int transformNo;
    int capabilities; // Renderer::CapabilityEnum bitfield
  };
  std::vector<DrawCall> _drawCalls;

  void addDrawCall(GLsizei count, GLenum mode, uint32_t modColor,
                   TextureID texID, float lineWidth, int cap) {
    int transformNo = int(_transforms.size())-1;
    if (!_drawCalls.empty()) {
      DrawCall& dc = _drawCalls.back();
      if (mode == dc.mode && modColor == dc.modColor && texID == dc.texID
          && lineWidth == dc.lineWidth && transformNo == dc.transformNo
          && cap == dc.capabilities) {
	dc.count += count;
	return;
      }
    }
    _drawCalls.push_back(
      {count, mode, modColor, texID, lineWidth, transformNo, cap});
  }

  int _currentGLCap = -1; // current GL capability state
  void setGLCapabilities(int cap);
  void setupBuffer();

  // prevent copy/assignment
  OpenGLRenderer(const OpenGLRenderer&) = delete;
  OpenGLRenderer& operator=(const OpenGLRenderer&) = delete;
};
