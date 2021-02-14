//
// Camera.hh
// Copyright (C) 2021 Richard Bradley
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


  // functions
  bool calcScreenOrthoProjection(float width, float height, Mat4& result);
    // simple orthogonal projection to work in screen coordinates
    // (0,0) upper left-hand corner, (width,height) lower right
}


class gx::Camera
{
 public:
  Camera() = default;

  // methods
  [[nodiscard]] const Vec3& pos() const { return _pos; }
  [[nodiscard]] const Vec3& dir() const { return _vnormal; }
  [[nodiscard]] const Vec3& vup() const { return _vup; }
  [[nodiscard]] float nearClip() const { return _nearClip; }
  [[nodiscard]] float farClip() const { return _farClip; }
  [[nodiscard]] float zoom() const { return _zoom; }
  [[nodiscard]] float fov() const { return _fov; }
  [[nodiscard]] CoordSystemType coordSystem() const { return _coordSystem; }
  [[nodiscard]] ProjectionType projection() const { return _projection; }

  void setPos(const Vec3& pos) { _pos = pos; }
  void setDir(const Vec3& dir) { _vnormal = dir; }
  void setVUP(const Vec3& vup) { _vup = vup; }

  inline void setViewByCOI(const Vec3& pos, const Vec3& coi);
  inline void setViewByCOI(const Vec3& pos, const Vec3& coi, const Vec3& vup);

  inline void setViewByDir(const Vec3& pos, const Vec3& dir);
  inline void setViewByDir(const Vec3& pos, const Vec3& dir, const Vec3& vup);

  void setClip(float near, float far) { _nearClip = near; _farClip = far; }
  void setZoom(float zoom) { _zoom = zoom; }
  void setFOV(float angle) { _fov = angle; }
  void setCoordSystem(CoordSystemType cs) { _coordSystem = cs; }
  void setProjection(ProjectionType pt) { _projection = pt; }

  bool calcView(Mat4& result) const;
  bool calcProjection(int width, int height, Mat4& result) const;

 private:
  // view config
  CoordSystemType _coordSystem = LEFT_HANDED;
  Vec3 _pos = {0,0,0};
  Vec3 _vnormal = {0,0,1};
  Vec3 _vup = {0,1,0};

  // projection config
  ProjectionType _projection = PERSPECTIVE;
  float _zoom = 1.0f, _fov = 90.0f;
  float _nearClip = 1.0f, _farClip = 1000.0f;
};


void gx::Camera::setViewByCOI(const Vec3& pos, const Vec3& coi)
{
  _pos = pos;
  _vnormal = coi;
  _vnormal -= _pos;
  _vnormal.normalize();
}

void gx::Camera::setViewByCOI(const Vec3& pos, const Vec3& coi, const Vec3& vup)
{
  _pos = pos;
  _vnormal = coi;
  _vnormal -= _pos;
  _vnormal.normalize();
  _vup = vup;
  _vup.normalize();
}

void gx::Camera::setViewByDir(const Vec3& pos, const Vec3& dir)
{
  _pos = pos;
  _vnormal = dir;
  _vnormal.normalize();
}

void gx::Camera::setViewByDir(const Vec3& pos, const Vec3& dir, const Vec3& vup)
{
  _pos = pos;
  _vnormal = dir;
  _vnormal.normalize();
  _vup = vup;
  _vup.normalize();
}
