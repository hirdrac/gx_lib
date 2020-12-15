//
// gx/OpenGLRenderer.hh
// Copyright (C) 2020 Richard Bradley
//
// FIXME - make TexID unique across all Renderer instances

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

  int maxTextureSize() override { return GLTexture2D::maxSize(); }
  int setTexture(int id, const Image& img, FilterType minFilter, FilterType magFilter) override;
  void freeTexture(int id) override { _textures.erase(id); }
  void setBGColor(float r, float g, float b) override { _bgColor.set(r,g,b); }

  void clearFrame(int width, int height) override;
  void draw(const DrawList& dl, const Color& modColor) override;
  void renderFrame() override;

 private:
  int _width = 0, _height = 0;
  Vec3 _bgColor = {0,0,0};

  GLProgram _sp0;
  GLUniformMat4f _sp0_trans;
  GLUniform4f _sp0_modColor;

  GLProgram _sp1;
  GLUniformMat4f _sp1_trans;
  GLUniform4f _sp1_modColor;
  GLUniform1i _sp1_texUnit;

  GLProgram _sp2;
  GLUniformMat4f _sp2_trans;
  GLUniform4f _sp2_modColor;
  GLUniform1i _sp2_texUnit;

  GLVertexArray _vao;
  GLBuffer _vbo;
  struct Vertex { float x,y,s,t; uint32_t c; };
  std::vector<Vertex> _vertices;

  struct TextureEntry {
    GLTexture2D tex;
    int flags = 0;
    int unit = -1;
    int shader = 0; // 1-mono, 2-color
  };
  std::unordered_map<int,TextureEntry> _textures;

  struct DrawCall {
    GLenum mode; // GL_LINES, GL_TRIANGLES
    GLsizei count;
    Color modColor;
    int texID;
    float lineWidth;
  };
  std::vector<DrawCall> _drawCalls;
  int lastTexID = 0;
  bool _changed = true;

  void addVertex(Vec2 pt, uint32_t c) {
    _vertices.push_back({pt.x,pt.y, 0.0f,0.0f, c}); }
  void addVertex(Vec2 pt, Vec2 tx, uint32_t c) {
    _vertices.push_back({pt.x,pt.y, tx.x,tx.y, c}); }

  void addDrawCall(GLenum mode, GLsizei count, const Color& modColor,
                   int texID, float lineWidth) {
    if (!_drawCalls.empty()) {
      DrawCall& dc = _drawCalls.back();
      if (mode == dc.mode && texID == dc.texID && lineWidth == dc.lineWidth) {
	dc.count += count;
	return;
      }
    }
    _drawCalls.push_back({mode, count, modColor, texID, lineWidth});
  }

  // prevent copy/assignment
  OpenGLRenderer(const OpenGLRenderer&) = delete;
  OpenGLRenderer& operator=(const OpenGLRenderer&) = delete;
};
