//
// gx/Camera.cc
// Copyright (C) 2023 Richard Bradley
//

#include "Camera.hh"
#include "Logger.hh"
using namespace gx;


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


// Camera class
bool Camera::updateView()
{
  const float dot = dotProduct(_vnormal, _vup);
  if (dot >= .99999) {
    GX_LOG_ERROR("bad vup: ", _vup);
    return false;
  }

  _vtop = unitVec(_vup - (_vnormal * dot));
  _vside = unitVec(
    (_coordSystem == LEFT_HANDED)
    ? crossProduct(_vtop, _vnormal) : crossProduct(_vnormal, _vtop));

  return true;
}

bool Camera::calcView(Mat4& result) const
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

bool Camera::calcProjection(
  int screenWidth, int screenHeight, Mat4& result) const
{
  if (!isPos(_zoom)) {
    GX_LOG_ERROR("bad zoom value: ", _zoom);
    return false;
  }

  if (!isPos(_fov) || isGTE(_fov, 180.0f)) {
    GX_LOG_ERROR("bad fov value: ", _fov);
    return false;
  }

  const float vlen = std::tan(degToRad(_fov * .5f)) / _zoom;
    // fov:90 == 1.0

  float vsideL = vlen, vtopL = vlen;
  if (screenWidth >= screenHeight) {
    vsideL *= float(screenWidth) / float(screenHeight);
  } else {
    vtopL *= float(screenHeight) / float(screenWidth);
  }

  if (_projection == PERSPECTIVE) {
    const float clipLen = _farClip - _nearClip;
    result = {
      1.0f / vsideL, 0, 0, 0,
      0, 1.0f / vtopL, 0, 0,
      0, 0, -(_farClip + _nearClip) / clipLen, -1.0f,
      0, 0, -(2.0f * _farClip * _nearClip) / clipLen, 0
    };
  } else {
    // FIXME: verify camera orthogonal projection
    result = {
      1.0f / vsideL, 0, 0, 0,
      0, 1.0f / vtopL, 0, 0,
      0, 0, 1.0f, 0,
      0, 0, 0, 1};
  }
  return true;
}

bool Camera::calcDirToScreenPt(
  int screenWidth, int screenHeight, Vec2 mousePt, Vec3& result) const
{
  const float sw = float(screenWidth), sh = float(screenHeight);
  const float vlen = std::tan(degToRad(_fov * .5f)) / _zoom;
  Vec3 vx = _vside * vlen;
  Vec3 vy = _vtop * vlen;
  if (screenWidth >= screenHeight) {
    vx *= sw / sh;
  } else {
    vy *= sh / sw;
  }

  const float cx = sw * .5f;
  const float cy = sh * .5f;

  // since we are calculating a direction, just assume eye is at origin
  // and view plane center is just 1 away from eye (in direction of vnormal)
  result = unitVec(
    _vnormal + (vx * ((mousePt.x - cx) / cx)) + (vy * -((mousePt.y - cy) / cy)));
  return true;
}
