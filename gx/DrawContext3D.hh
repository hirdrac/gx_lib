//
// gx/DrawContext3D.hh
// Copyright (C) 2026 Richard Bradley
//

#pragma once
#include "DrawList.hh"
#include "Renderer.hh"
#include "Color.hh"
#include "Normal.hh"
#include "Types.hh"


class gx::DrawContext3D
{
 public:
  explicit DrawContext3D(DrawList& dl) : _dl{&dl} { init(); }

  // Low-level data entry
  void clearList() { init(); _dl->clear(); }
  void append(const DrawList& dl) { init(); _dl->append(dl); }

  // Context state change (reset for every DrawList)
  void color(float r, float g, float b, float a = 1.0f) {
    color(packRGBA8(r, g, b, a)); }
  void color(const Vec3& c, float a = 1.0f) { color(packRGBA8(c, a)); }
  void color(const Vec4& c) { color(packRGBA8(c)); }
  void color(RGBA8 c) { _color0 = c; }

  void changeAlpha(float a);
    // change alpha value of current color

  void texture(const TextureHandle& h) { texture(h.id()); }
  void texture(TextureID tid) {
    if (tid != _lastTexID) { _lastTexID = tid; _dl->texture(tid); } }

  void normal(float x, float y, float z) { normal(packNormal(x,y,z)); }
  void normal(const Vec3& n) { normal(packNormal(n)); }
  void normal(uint32_t n) {
    if (n != _lastNormal) { _lastNormal = n; _dl->normal(n); } }

  // Render state change (persists across different DrawLists)
  void lineWidth(float w) { _dl->lineWidth(w); }

  void modColor(float r, float g, float b, float a = 1.0f) {
    modColor(packRGBA8(r, g, b, a)); }
  void modColor(const Vec3& c, float a = 1.0f) { modColor(packRGBA8(c, a)); }
  void modColor(const Vec4& c) { modColor(packRGBA8(c)); }
  void modColor(RGBA8 c) { _dl->modColor(c); }

  void capabilities(int32_t c) { _dl->capabilities(c); }

  // Camera
  void camera(const Mat4& viewT, const Mat4& projT) {
    _dl->camera(viewT, projT); }

  // Lighting
  void light(const Vec3& pos, const Vec3& ambient, const Vec3& diffuse) {
    _dl->light(pos, ambient, diffuse); }

  // View
  void viewport(int x, int y, int w, int h) {
    _dl->viewport(x, y, w, h); }
  void viewportFull() {
    _dl->viewportFull(); }
  void clearView(float r, float g, float b) {
    clearView(packRGBA8(r,g,b,1.0f)); }
  void clearView(const Vec3& c, float a = 1.0f) { clearView(packRGBA8(c, a)); }
  void clearView(const Vec4& c) { clearView(packRGBA8(c)); }
  void clearView(RGBA8 c) { _dl->clearView(c); }

  // Line drawing
  void line(const Vec3& a, const Vec3& b);
  void line(const Vertex3C& a, const Vertex3C& b) {
    _dl->line3C(a, b); }

  void lineStart(const Vec3& a);
  void lineStart(const Vertex3C& a) {
    _dl->lineStart3C(a); }
  void lineTo(const Vec3& a);
  void lineTo(const Vertex3C& a) {
    _dl->lineTo3C(a); }

  template<class T1, class T2, class T3, class... Args>
  void line(const T1& p1, const T2& p2, const T3& p3, Args&&... args) {
    lineStart(p1);
    lineTo(p2);
    lineTo(p3);
    (lineTo(args),...);
  }

  template<class T1, class T2, class T3, class... Args>
  void lineLoop(const T1& p1, const T2& p2, const T3& p3, Args&&... args) {
    line(p1, p2, p3, args..., p1); }

  // Poly drawing
  // Triangle  Quad
  //   A--B    A--B
  //   | /     | /|
  //   |/      |/ |
  //   C       C--D
  void triangle(const Vec3& a, const Vec3& b, const Vec3& c);
  void triangle(const Vertex3T& a, const Vertex3T& b, const Vertex3T& c);
  void triangle(const Vertex3C& a, const Vertex3C& b, const Vertex3C& c) {
    _dl->triangle3C(a, b, c); }
  void triangle(const Vertex3TC& a, const Vertex3TC& b, const Vertex3TC& c) {
    _dl->triangle3TC(a, b, c); }

  void quad(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d);
  void quad(const Vertex3T& a, const Vertex3T& b,
            const Vertex3T& c, const Vertex3T& d);
  void quad(const Vertex3C& a, const Vertex3C& b,
            const Vertex3C& c, const Vertex3C& d) {
    _dl->quad3C(a, b, c, d); }
  void quad(const Vertex3TC& a, const Vertex3TC& b,
            const Vertex3TC& c, const Vertex3TC& d) {
    _dl->quad3TC(a, b, c, d); }

  // Data extraction
  [[nodiscard]] const DrawList& drawList() const { return *_dl; }

 private:
  DrawList* _dl = nullptr;

  // general properties
  TextureID _lastTexID;
  uint32_t _lastNormal;
  RGBA8 _color0;     // current color
  RGBA8 _dataColor;  // last color set in data

  void init() {
    _lastTexID = 0;
    _lastNormal = 0;
    _color0 = 0;
    _dataColor = 0;
  }

  bool checkColor() {
    if ((_color0 & 0xff000000) == 0) { return false; }
    if (_dataColor != _color0) {
      _dataColor = _color0;
      _dl->color(_color0);
    }
    return true; // proceed with draw
  }
};
