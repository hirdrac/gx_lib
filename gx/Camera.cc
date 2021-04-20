//
// Camera.cc
// Copyright (C) 2021 Richard Bradley
//

#include "Camera.hh"
#include "Logger.hh"


// **** NOTES ****
// all projections for OpenGL coords for now
// (Vulkan to be supported at some future date)
//
// OpenGL coords (left handed)
//   (-1,-1) at bottom left corner
//   Z coord: -1 to 1
//
// Vulkan coords (right handed)
//   (-1,-1) at top left corner
//   Z coord: 0 to 1


// functions
bool gx::calcOrthoProjection(float width, float height, Mat4& result)
{
  // simple ortho projection for 2d rendering in screen coords
  //  x:[0 width] => x:[-1 1]
  //  y:[0 height]   y:[-1 1]
  // origin in upper left corner
  result = {
    2.0f / width, 0, 0, 0,
    0, -2.0f / height, 0, 0,
    //  negative value flips vertial direction for OpenGL
    0, 0, 1, 0,
    -1, 1, 0, 1 };

  return true;
}

// Camera class
bool gx::Camera::updateView()
{
  float dot = DotProduct(_vnormal, _vup);
  if (dot >= .99999) {
    GX_LOG_ERROR("bad vup");
    return false;
  }

  _vtop = _vup - (_vnormal * dot);
  _vtop.normalize();

  if (_coordSystem == LEFT_HANDED) {
    _vside = CrossProduct(_vtop, _vnormal);
  } else {
    _vside = CrossProduct(_vnormal, _vtop);
  }
  _vside.normalize();
  return true;
}

bool gx::Camera::calcView(Mat4& result) const
{
  result.setTranslation(-_pos.x, -_pos.y, -_pos.z);
  result *= { // 'lookAt' transform
    _vside.x, _vtop.x, -_vnormal.x, 0,
    _vside.y, _vtop.y, -_vnormal.y, 0,
    _vside.z, _vtop.z, -_vnormal.z, 0,
    0,        0,        0,          1.0f
  };
  return true;
}

bool gx::Camera::calcProjection(
  int screenWidth, int screenHeight, Mat4& result) const
{
  if (!isPos(_zoom)) {
    GX_LOG_ERROR("bad zoom value: ", _zoom);
    return false;
  }

  if (!isPos(_fov) || isGTE(_fov,180.0f)) {
    GX_LOG_ERROR("bad fov value: ", _fov);
    return false;
  }

  float vlen = std::tan(degToRad(_fov / 2.0f)) / _zoom;
    // fov:90 == 1.0

  float vsideL = vlen, vtopL = vlen;
  if (screenWidth >= screenHeight) {
    vsideL *= float(screenWidth) / float(screenHeight);
  } else {
    vtopL *= float(screenHeight) / float(screenWidth);
  }

  if (_projection == PERSPECTIVE) {
    float clipLen = _farClip - _nearClip;
    result = {
      1.0f / vsideL, 0, 0, 0,
      0, 1.0f / vtopL, 0, 0,
      0, 0, -(_farClip + _nearClip) / clipLen, -1.0f,
      0, 0, -(2.0f * _farClip * _nearClip) / clipLen, 0
    };
  } else {
    // FIXME - verify camera orthogonal projection
    result = {
      1.0f / vsideL, 0, 0, 0,
      0, 1.0f / vtopL, 0, 0,
      0, 0, 1.0f, 0,
      0, 0, 0, 1};
  }
  return true;
}

bool gx::Camera::calcDirToScreenPt(
  int screenWidth, int screenHeight,
  float mouseX, float mouseY, Vec3& result) const
{
  float sw = float(screenWidth), sh = float(screenHeight);
  float vlen = std::tan(degToRad(_fov / 2.0f)) / _zoom;
  Vec3 vx = _vside * vlen;
  Vec3 vy = _vtop * vlen;
  if (screenWidth >= screenHeight) {
    vx *= sw / sh;
  } else {
    vy *= sh / sw;
  }

  float cx = sw / 2.0f;
  float cy = sh / 2.0f;

  // since we are calculating a direction, just assume eye is at origin
  // and view plane center is just 1 away from eye (in direction of vnormal)
  result = _vnormal
    + (vx * ((mouseX - cx) / cx)) + (vy * -((mouseY - cy) / cy));
  result.normalize();
  return true;
}
