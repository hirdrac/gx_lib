//
// gx/DrawList.hh
// Copyright (C) 2024 Richard Bradley
//

#pragma once
#include "DrawEntry.hh"
#include "Types.hh"
#include <initializer_list>
#include <vector>

namespace gx {
  class DrawList;

  struct Vertex2C {
    float x, y; uint32_t c;

    Vertex2C() = default;
    constexpr Vertex2C(Vec2 pos, uint32_t col)
      : x{pos.x}, y{pos.y}, c{col} { }
    constexpr Vertex2C(float px, float py, uint32_t col)
      : x{px}, y{py}, c{col} { }
  };

  struct Vertex2T { float x, y, s, t; };
  struct Vertex2TC { float x, y, s, t; uint32_t c; };

  struct Vertex3C { float x, y, z; uint32_t c; };
  struct Vertex3T { float x, y, z, s, t; };
  struct Vertex3TC { float x, y, z, s, t; uint32_t c; };

  struct Vertex3TCN { float x,y,z,s,t; uint32_t c, n; };
}

class gx::DrawList
{
 public:
  [[nodiscard]] const Value* data() const { return _data.data(); }
  [[nodiscard]] std::size_t size() const { return _data.size(); }
  [[nodiscard]] bool empty() const { return _data.empty(); }

  void clear() { _data.clear(); }

  void append(const DrawList& dl) {
    const Value* d = dl.data();
    _data.insert(_data.end(), d, d + dl.size());
  }

  // raw draw commands
  void viewport(int32_t x, int32_t y, int32_t w, int32_t h) {
    add(CMD_viewport, x, y, w, h); }
  void viewportFull() {
    add(CMD_viewportFull); }
  void color(uint32_t c) {
    add(CMD_color, c); }
  void texture(uint32_t t) {
    add(CMD_texture, t); }
  void lineWidth(float w) {
    add(CMD_lineWidth, w); }
  void normal(uint32_t n) {
    add(CMD_normal, n); }
  void modColor(uint32_t c) {
    add(CMD_modColor, c); }
  void capabilities(int32_t c) {
    add(CMD_capabilities, c); }
  void camera(const Mat4& m1, const Mat4& m2) {
    add(CMD_camera, m1, m2); }
  void cameraReset() {
    add(CMD_cameraReset); }
  void light(const Vec3& pt, uint32_t a, uint32_t d) {
    add(CMD_light, pt.x, pt.y, pt.z, a, d); }
  void clearView(uint32_t c) {
    add(CMD_clearView, c); }
  void line2(Vec2 a, Vec2 b) {
    add(CMD_line2, a.x, a.y, b.x, b.y); }
  void line3(const Vec3& a, const Vec3& b) {
    add(CMD_line3, a.x, a.y, a.z, b.x, b.y, b.z); }
  void line2C(const Vertex2C& a, const Vertex2C& b) {
    add(CMD_line2C, a.x, a.y, a.c, b.x, b.y, b.c); }
  void line3C(const Vertex3C& a, const Vertex3C& b) {
    add(CMD_line3C, a.x, a.y, a.z, a.c, b.x, b.y, b.z, b.c); }
  void lineStart2(Vec2 a) {
    add(CMD_lineStart2, a.x, a.y); }
  void lineTo2(Vec2 a) {
    add(CMD_lineTo2, a.x, a.y); }
  void lineStart3(const Vec3& a) {
    add(CMD_lineStart3, a.x, a.y, a.z); }
  void lineTo3(const Vec3& a) {
    add(CMD_lineTo3, a.x, a.y, a.z); }
  void lineStart2C(const Vertex2C& a) {
    add(CMD_lineStart2C, a.x, a.y, a.c); }
  void lineTo2C(const Vertex2C& a) {
    add(CMD_lineTo2C, a.x, a.y, a.c); }
  void lineStart3C(const Vertex3C& a) {
    add(CMD_lineStart3C, a.x, a.y, a.z, a.c); }
  void lineTo3C(const Vertex3C& a) {
    add(CMD_lineTo3C, a.x, a.y, a.z, a.c); }
  void triangle2(Vec2 a, Vec2 b, Vec2 c) {
    add(CMD_triangle2, a.x, a.y, b.x, b.y, c.x, c.y); }
  void triangle3(const Vec3& a, const Vec3& b, const Vec3& c) {
    add(CMD_triangle3, a.x, a.y, a.z, b.x, b.y, b.z, c.x, c.y, c.z); }
  void triangle2T(const Vertex2T& a, const Vertex2T& b, const Vertex2T& c) {
    add(CMD_triangle2T, a.x, a.y, a.s, a.t, b.x, b.y, b.s, b.t,
        c.x, c.y, c.s, c.t); }
  void triangle3T(const Vertex3T& a, const Vertex3T& b, const Vertex3T& c) {
    add(CMD_triangle2T, a.x, a.y, a.z, a.s, a.t, b.x, b.y, b.z, b.s, b.t,
        c.x, c.y, c.z, c.s, c.t); }
  void triangle2C(const Vertex2C& a, const Vertex2C& b, const Vertex2C& c) {
    add(CMD_triangle2C, a.x, a.y, a.c, b.x, b.y, b.c, c.x, c.y, c.c); }
  void triangle3C(const Vertex3C& a, const Vertex3C& b, const Vertex3C& c) {
    add(CMD_triangle3C, a.x, a.y, a.z, a.c, b.x, b.y, b.z, b.c,
        c.x, c.y, c.z, c.c); }
  void triangle2TC(const Vertex2TC& a, const Vertex2TC& b, const Vertex2TC& c) {
    add(CMD_triangle2TC, a.x, a.y, a.s, a.t, a.c, b.x, b.y, b.s, b.t, b.c,
        c.x, c.y, c.s, c.t, c.c); }
  void triangle3TC(const Vertex3TC& a, const Vertex3TC& b, const Vertex3TC& c) {
    add(CMD_triangle3TC, a.x, a.y, a.z, a.s, a.t, a.c,
        b.x, b.y, b.z, b.s, b.t, b.c, c.x, c.y, c.z, c.s, c.t, c.c); }
  void triangle3TCN(const Vertex3TCN& a, const Vertex3TCN& b,
                    const Vertex3TCN& c) {
    add(CMD_triangle3TCN, a.x, a.y, a.z, a.s, a.t, a.c, a.n,
        b.x, b.y, b.z, b.s, b.t, b.c, b.n, c.x, c.y, c.z, c.s, c.t, c.c, c.n); }
  void quad2(Vec2 a, Vec2 b, Vec2 c, Vec2 d) {
    add(CMD_quad2, a.x, a.y, b.x, b.y, c.x, c.y, d.x, d.y); }
  void quad3(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d) {
    add(CMD_quad3, a.x, a.y, a.z, b.x, b.y, b.z, c.x, c.y, c.z, d.x, d.y, d.z); }
  void quad2T(const Vertex2T& a, const Vertex2T& b, const Vertex2T& c,
              const Vertex2T& d) {
    add(CMD_quad2T, a.x, a.y, a.s, a.t, b.x, b.y, b.s, b.t,
        c.x, c.y, c.s, c.t, d.x, d.y, d.s, d.t); }
  void quad3T(const Vertex3T& a, const Vertex3T& b, const Vertex3T& c,
              const Vertex3T& d) {
    add(CMD_quad3T, a.x, a.y, a.z, a.s, a.t, b.x, b.y, b.z, b.s, b.t,
        c.x, c.y, c.z, c.s, c.t, d.x, d.y, d.z, d.s, d.t); }
  void quad2C(const Vertex2C& a, const Vertex2C& b, const Vertex2C& c,
              const Vertex2C& d) {
    add(CMD_quad2C, a.x, a.y, a.c, b.x, b.y, b.c, c.x, c.y, c.c,
        d.x, d.y, d.c); }
  void quad3C(const Vertex3C& a, const Vertex3C& b, const Vertex3C& c,
              const Vertex3C& d) {
    add(CMD_quad3C, a.x, a.y, a.z, a.c, b.x, b.y, b.z, b.c, c.x, c.y, c.z, c.c,
        d.x, d.y, d.z, d.c); }
  void quad2TC(const Vertex2TC& a, const Vertex2TC& b, const Vertex2TC& c,
               const Vertex2TC& d) {
    add(CMD_quad2TC, a.x, a.y, a.s, a.t, a.c, b.x, b.y, b.s, b.t, b.c,
        c.x, c.y, c.s, c.t, c.c, d.x, d.y, d.s, d.t, d.c); }
  void quad3TC(const Vertex3TC& a, const Vertex3TC& b, const Vertex3TC& c,
               const Vertex3TC& d) {
    add(CMD_quad3TC, a.x, a.y, a.z, a.s, a.t, a.c, b.x, b.y, b.z, b.s, b.t, b.c,
        c.x, c.y, c.z, c.s, c.t, c.c, d.x, d.y, d.z, d.s, d.t, d.c); }
  void quad3TCN(const Vertex3TCN& a, const Vertex3TCN& b, const Vertex3TCN& c,
                const Vertex3TCN& d) {
    add(CMD_quad3TCN, a.x, a.y, a.z, a.s, a.t, a.c, a.n,
        b.x, b.y, b.z, b.s, b.t, b.c, b.n, c.x, c.y, c.z, c.s, c.t, c.c, c.n,
        d.x, d.y, d.z, d.s, d.t, d.c, d.n); }
  void rectangle(Vec2 a, Vec2 b) {
    add(CMD_rectangle, a.x, a.y, b.x, b.y); }
  void rectangleT(const Vertex2T& a, const Vertex2T& b) {
    add(CMD_rectangleT, a.x, a.y, a.s, a.t, b.x, b.y, b.s, b.t); }

 private:
  std::vector<Value> _data;

  void add(DrawCmd cmd, const Mat4& m1, const Mat4& m2) {
    _data.reserve(_data.size() + 33);
    _data.push_back(cmd);
    _data.insert(_data.end(), m1.begin(), m1.end());
    _data.insert(_data.end(), m2.begin(), m2.end());
  }

  template<class... Args>
  void add(DrawCmd cmd, const Args&... args) {
    if constexpr (sizeof...(args) == 0) {
      _data.push_back(cmd);
    } else {
      const std::initializer_list<Value> x {cmd, args...};
      _data.insert(_data.end(), x.begin(), x.end());
    }
  }
};
