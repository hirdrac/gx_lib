//
// gx/DrawContext3D.cc
// Copyright (C) 2026 Richard Bradley
//

#include "DrawContext3D.hh"
using namespace gx;


void DrawContext3D::changeAlpha(float a)
{
  const RGBA8 val = RGBA8(std::clamp(a, 0.0f, 1.0f) * 255.0f + .5f) << 24;
  _color0 = (_color0 & ~0xff000000) | val;
}

void DrawContext3D::line(const Vec3& a, const Vec3& b)
{
  if (checkColor()) { _dl->line3(a, b); }
}

void DrawContext3D::lineStart(const Vec3& a)
{
  if (checkColor()) { _dl->lineStart3(a); }
}

void DrawContext3D::lineTo(const Vec3& a)
{
  if (checkColor()) { _dl->lineTo3(a); }
}

void DrawContext3D::triangle(const Vec3& a, const Vec3& b, const Vec3& c)
{
  if (checkColor()) { _dl->triangle3(a, b, c); }
}

void DrawContext3D::triangle(
  const Vertex3T& a, const Vertex3T& b, const Vertex3T& c)
{
  if (checkColor()) { _dl->triangle3T(a, b, c); }
}

void DrawContext3D::quad(
  const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d)
{
  if (checkColor()) { _dl->quad3(a, b, c, d); }
}

void DrawContext3D::quad(
  const Vertex3T& a, const Vertex3T& b, const Vertex3T& c, const Vertex3T& d)
{
  if (checkColor()) { _dl->quad3T(a, b, c, d); }
}
