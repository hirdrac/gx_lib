//
// gx/Camera.cc
// Copyright (C) 2025 Richard Bradley
//

#include "Camera.hh"
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
bool Camera::setView(const Vec3& p, const Vec3& vn, const Vec3& vu)
{
  const float dot = dotProduct(vn, vu);
  if (dot >= .99999) { return false; }

  _pos = p;
  _vnormal = vn;
  _vup = vu;

  _vtop = unitVec(_vup - (_vnormal * dot));
  _vside = unitVec(
    (_coordSystem == LEFT_HANDED)
    ? crossProduct(_vtop, _vnormal) : crossProduct(_vnormal, _vtop));
  return true;
}

bool Camera::setViewByCOI(const Vec3& pos, const Vec3& coi, const Vec3& vup)
{
  return setView(pos, unitVec(coi - pos), unitVec(vup));
}

bool Camera::setViewByDir(const Vec3& pos, const Vec3& dir, const Vec3& vup)
{
  return setView(pos, unitVec(dir), unitVec(vup));
}

bool Camera::setFOV(float angle)
{
  if (!isPos(angle) || isGTE(angle, 180.0f)) { return false; }

  _fov = angle;
  _vlen = std::tan(degToRad(angle * .5f));  // fov:90 == 1.0
  return true;
}

bool Camera::setZoom(float zoom)
{
  if (!isPos(zoom)) { return false; }

  _zoom = zoom;
  return true;
}

bool Camera::calcView(Mat4& result) const
{
  //result.setTranslation(-_pos.x, -_pos.y, -_pos.z);
  //result *= { // 'lookAt' transform
  //  _vside.x, _vtop.x, -_vnormal.x, 0,
  //  _vside.y, _vtop.y, -_vnormal.y, 0,
  //  _vside.z, _vtop.z, -_vnormal.z, 0,
  //  0,        0,        0,          1.0f
  //};

  result = {
    _vside.x, _vtop.x, -_vnormal.x, 0,
    _vside.y, _vtop.y, -_vnormal.y, 0,
    _vside.z, _vtop.z, -_vnormal.z, 0,
    -dotProduct(_pos, _vside),
    -dotProduct(_pos, _vtop),
    dotProduct(_pos, _vnormal), 1.0f};
  return true;
}

bool Camera::calcProjection(Mat4& result) const
{
  float width, height;
  if (_vpSet) {
    width = _vpWidth; height = _vpHeight;
  } else {
    width = _screenWidth; height = _screenHeight;
  }

  const float len = vlen();
  float vsideL = len, vtopL = len;
  if (width >= height) {
    vsideL *= width / height;
  } else {
    vtopL *= height / width;
  }

  float offsetX = 0, offsetY = 0;
  if (_vpSet) {
    offsetX = (_vpX / _screenWidth) * 2.0f;
    offsetY = (_vpY / _screenHeight) * 2.0f;
    vsideL *= ((_screenWidth - _vpWidth) / _screenWidth);
    vtopL *= ((_screenHeight - _vpHeight) / _screenHeight);
  }

  const float clipLen = _farClip - _nearClip;
  if (_projection == PERSPECTIVE) {
    result = {
      1.0f / vsideL, 0, 0, 0,
      0, 1.0f / vtopL, 0, 0,
      -offsetX, -offsetY, -(_farClip + _nearClip) / clipLen, -1.0f,
      0, 0, -(2.0f * _farClip * _nearClip) / clipLen, 0
    };
  } else {
    result = {
      1.0f / vsideL, 0, 0, 0,
      0, 1.0f / vtopL, 0, 0,
      0, 0, -(_farClip + _nearClip) / clipLen, 0,
      offsetX, offsetY, -(2.0f * _farClip * _nearClip) / clipLen, 1.0f
    };
  }
  return true;
}

Vec3 Camera::dirToScreenPt(Vec2 mousePt) const
{
  float width, height;
  if (_vpSet) {
    width = _vpWidth; height = _vpHeight;
  } else {
    width = _screenWidth; height = _screenHeight;
  }

  const float len = vlen();
  float vsideL = len, vtopL = len;
  if (width >= height) {
    vsideL *= width / height;
  } else {
    vtopL *= height / width;
  }

  const Vec3 vx = _vside * vsideL;
  const Vec3 vy = _vtop * vtopL;
  const float cx = _screenWidth * .5f;
  const float cy = _screenHeight * .5f;

  // FIXME: finish support for viewport

  // since we are calculating a direction, just assume eye is at origin
  // and view plane center is just 1 away from eye (in direction of vnormal)
  return unitVec(
    _vnormal + (vx * ((mousePt.x - cx) / cx)) + (vy * -((mousePt.y - cy) / cy)));
}
