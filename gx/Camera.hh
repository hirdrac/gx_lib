//
// gx/Camera.hh
// Copyright (C) 2022 Richard Bradley
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
  constexpr Mat4 orthoProjection(float width, float height);
    // simple orthogonal projection to work in screen coordinates
    // (0,0) upper left-hand corner, (width,height) lower right
}


class gx::Camera
{
 public:
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

  inline bool setViewByCOI(const Vec3& pos, const Vec3& coi, const Vec3& vup);
  inline bool setViewByDir(const Vec3& pos, const Vec3& dir, const Vec3& vup);

  void setClip(float near, float far) { _nearClip = near; _farClip = far; }
  void setZoom(float zoom) { _zoom = zoom; }
  void setFOV(float angle) { _fov = angle; }
  void setCoordSystem(CoordSystemType cs) { _coordSystem = cs; }
  void setProjection(ProjectionType pt) { _projection = pt; }

  bool calcView(Mat4& result) const;
  bool calcProjection(int screenWidth, int screenHeight, Mat4& result) const;

  bool calcDirToScreenPt(int screenWidth, int screenHeight,
                         float mouseX, float mouseY, Vec3& result) const;
    // eye to screen point direction calc
    // use {eye, result} ray intersection for mouse selection calcs

 private:
  // view config
  CoordSystemType _coordSystem = LEFT_HANDED;
  Vec3 _pos = {0,0,0};
  Vec3 _vnormal = {0,0,1};
  Vec3 _vup = {0,1,0};
  Vec3 _vtop = {0,1,0};
  Vec3 _vside = {1,0,0};

  // projection config
  ProjectionType _projection = PERSPECTIVE;
  float _zoom = 1.0f, _fov = 90.0f;
  float _nearClip = 1.0f, _farClip = 1000.0f;

  bool updateView();
};


// **** Inline Implementations ****
bool gx::Camera::setViewByCOI(const Vec3& pos, const Vec3& coi, const Vec3& vup)
{
  _pos = pos;
  _vnormal = unitVec(coi - pos);
  _vup = unitVec(vup);
  return updateView();
}

bool gx::Camera::setViewByDir(const Vec3& pos, const Vec3& dir, const Vec3& vup)
{
  _pos = pos;
  _vnormal = unitVec(dir);
  _vup = unitVec(vup);
  return updateView();
}


constexpr gx::Mat4 gx::orthoProjection(float width, float height)
{
  // simple ortho projection for 2d rendering in screen coords
  //  x:[0 width] => x:[-1 1]
  //  y:[0 height]   y:[-1 1]
  // origin in upper left corner
  return {
    2.0f / width, 0, 0, 0,
    0, -2.0f / height, 0, 0,
    // negative value flips vertical direction for OpenGL
    // FIXME: handle vertical flip inside OpenGLRenderer in the future
    0, 0, 1, 0,
    -1, 1, 0, 1};
}
