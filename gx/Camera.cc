//
// Camera.cc
// Copyright (C) 2021 Richard Bradley
//

#include "Camera.hh"


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
bool gx::Camera::calcView(Mat4& result) const
{
  float dot = DotProduct(_vnormal, _vup);
  if (dot >= .99999) { return false; }

  Vec3 vtop = _vup - (_vnormal * dot);
  vtop.normalize();

  Vec3 vside;
  if (_coordSystem == LEFT_HANDED) {
    vside = CrossProduct(vtop, _vnormal);
  } else {
    vside = CrossProduct(_vnormal, vtop);
  }
  vside.normalize();

  result.setTranslation(-_pos.x, -_pos.y, -_pos.z);
  result *= { // 'lookAt' transform
    vside.x, vtop.x, -_vnormal.x, 0,
    vside.y, vtop.y, -_vnormal.y, 0,
    vside.z, vtop.z, -_vnormal.z, 0,
    0,       0,       0,          1.0f
  };
  return true;
}

bool gx::Camera::calcProjection(int width, int height, Mat4& result) const
{
  if (IsZero(_zoom)) { return false; }

  float vwidth, vheight;
  if (width >= height) {
    vwidth = float(width) / float(height);
    vheight = 1.0f;
  } else {
    vwidth = 1.0f;
    vheight = float(height) / float(width);
  }

  float vsideL = vwidth / _zoom;
  float vtopL = vheight / _zoom;
  if (_projection == PERSPECTIVE) {
    float clipLen = _farClip - _nearClip;
    result = {
      1.0f / vsideL, 0, 0, 0,
      0, 1.0f / vtopL, 0, 0,
      0, 0, -(_farClip + _nearClip) / clipLen, -1.0f,
      0, 0, -(2.0f * _farClip * _nearClip) / clipLen, 0
    };
  } else {
    result = {
      1.0f / vsideL, 0, 0, 0,
      0, 1.0f / vtopL, 0, 0,
      0, 0, 1.0f, 0,
      0, 0, 0, 1};
  }
  return true;
}
