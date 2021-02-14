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
  void setTransform(const Mat4& view, const Mat4& proj) override;
  void draw(const DrawEntry* data, std::size_t dataSize,
            const Color& modColor) override;
  void renderFrame() override;

 private:
  int _width = 0, _height = 0;
  Vec3 _bgColor = {0,0,0};

  GLProgram _sp[3];
  GLUniformMat4f _sp_trans[3];
  GLUniform4f _sp_modColor[3];
  GLUniform1i _sp_texUnit[3];

  GLVertexArray _vao;
  GLBuffer _vbo;
  std::vector<Vertex3TC> _vertices;

  struct TextureEntry {
    GLTexture2D tex;
    int flags = 0;
    int unit = -1;
    int shader = 0; // 1-mono, 2-color
  };
  std::unordered_map<TextureID,TextureEntry> _textures;

  struct TransformEntry {
    Mat4 view, projection;
  };
  std::vector<TransformEntry> _transforms;

  struct DrawCall {
    GLenum mode; // GL_LINES, GL_TRIANGLES
    GLsizei count;
    Color modColor;
    TextureID texID;
    float lineWidth;
    int transformNo;
    int capabilities; // Renderer::CapabilityEnum bitfield
  };
  std::vector<DrawCall> _drawCalls;
  TextureID _lastTexID = 0;
  bool _changed = true;

  void addVertex(Vec2 pt, uint32_t c) {
    _vertices.push_back({pt.x,pt.y,0.0f, 0.0f,0.0f, c}); }
  void addVertex(Vec3 pt, uint32_t c) {
    _vertices.push_back({pt.x,pt.y,pt.z, 0.0f,0.0f, c}); }
  void addVertex(Vec2 pt, Vec2 tx, uint32_t c) {
    _vertices.push_back({pt.x,pt.y,0.0f, tx.x,tx.y, c}); }
  void addVertex(Vec3 pt, Vec2 tx, uint32_t c) {
    _vertices.push_back({pt.x,pt.y,pt.z, tx.x,tx.y, c}); }

  void addDrawCall(GLenum mode, GLsizei count, const Color& modColor,
                   TextureID texID, float lineWidth) {
    int transformNo = int(_transforms.size())-1;
    if (!_drawCalls.empty()) {
      DrawCall& dc = _drawCalls.back();
      if (mode == dc.mode && texID == dc.texID && lineWidth == dc.lineWidth
          && transformNo == dc.transformNo && _drawCap == dc.capabilities) {
	dc.count += count;
	return;
      }
    }
    _drawCalls.push_back(
      {mode, count, modColor, texID, lineWidth, transformNo, _drawCap});
  }

  int _currentGLCap = -1; // current GL capability state
  void setGLCapabilities(int cap);

  // prevent copy/assignment
  OpenGLRenderer(const OpenGLRenderer&) = delete;
  OpenGLRenderer& operator=(const OpenGLRenderer&) = delete;
};
