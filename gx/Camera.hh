//
// gx/Camera.hh
// Copyright (C) 2024 Richard Bradley
//

#pragma once
#include "Types.hh"


namespace gx {
  class Camera;

  // enumerations
  enum CoordSystemType { LEFT_HANDED, RIGHT_HANDED };
    // LEFT_HANDED   RIGHT_HANDED
    // +Y  +Z        +Y  -Z
    //  | /           | /
    //  O-- +X        O-- +X

  enum ProjectionType { ORTHOGONAL, PERSPECTIVE };
}


class gx::Camera
{
 public:
  // view methods
  [[nodiscard]] CoordSystemType coordSystem() const { return _coordSystem; }
  [[nodiscard]] const Vec3& pos() const { return _pos; }
  [[nodiscard]] const Vec3& dir() const { return _vnormal; }
  [[nodiscard]] const Vec3& vup() const { return _vup; }
  [[nodiscard]] const Vec3& vtop() const { return _vtop; }
  [[nodiscard]] const Vec3& vside() const { return _vside; }

  void setCoordSystem(CoordSystemType cs) { _coordSystem = cs; }
  bool setViewByCOI(const Vec3& pos, const Vec3& coi, const Vec3& vup);
  bool setViewByDir(const Vec3& pos, const Vec3& dir, const Vec3& vup);

  bool calcView(Mat4& result) const;

  // projection methods
  [[nodiscard]] ProjectionType projection() const { return _projection; }
  [[nodiscard]] float nearClip() const { return _nearClip; }
  [[nodiscard]] float farClip() const { return _farClip; }
  [[nodiscard]] float zoom() const { return _zoom; }
  [[nodiscard]] float fov() const { return _fov; }
  [[nodiscard]] float screenWidth() const { return _screenWidth; }
  [[nodiscard]] float screenHeight() const { return _screenHeight; }

  void setProjection(ProjectionType pt) { _projection = pt; }
  bool setFOV(float angle);
  bool setZoom(float zoom);
  void setClip(float near, float far) { _nearClip = near; _farClip = far; }
  void setScreen(float width, float height) {
    _screenWidth = width; _screenHeight = height; }
  void setViewport(float x, float y, float width, float height) {
    _vpSet = true; _vpX = x; _vpY = y; _vpWidth = width; _vpHeight = height; }
  void clearViewport() { _vpSet = false; }

  template<class T>
  void setScreen(const T& x) { setScreen(float(x.width()), float(x.height())); }

  bool calcProjection(Mat4& result) const;

  // other methods
  [[nodiscard]] Vec3 dirToScreenPt(Vec2 mousePt) const;
    // eye to screen point direction calc
    // use {eye, result} ray intersection for mouse selection calcs

 private:
  // view config
  CoordSystemType _coordSystem = LEFT_HANDED;
  Vec3 _pos{0,0,0}, _vnormal{0,0,1}, _vup{0,1,0};
  Vec3 _vtop{0,1,0}, _vside{1,0,0};

  // projection config
  ProjectionType _projection = PERSPECTIVE;
  float _zoom = 1.0f, _fov = 90.0f, _vlen = 1.0f;
  float _nearClip = 1.0f, _farClip = 1000.0f;
  float _screenWidth = 1.0f, _screenHeight = 1.0f;
  float _vpX = 0.0f, _vpY = 0.0f, _vpWidth = 0.0f, _vpHeight = 0.0f;
  bool _vpSet = false;

  bool setView(const Vec3& p, const Vec3& vn, const Vec3& vu);

  [[nodiscard]] float vlen() const {
    return ((_projection == PERSPECTIVE) ? _vlen : 1.0f) / _zoom;
  }
};
