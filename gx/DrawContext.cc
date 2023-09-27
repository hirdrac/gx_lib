//
// gx/DrawContext.cc
// Copyright (C) 2023 Richard Bradley
//

#include "DrawContext.hh"
#include "Font.hh"
#include "Unicode.hh"
#include "MathUtil.hh"
#include "Assert.hh"
using namespace gx;


namespace {
  [[nodiscard]] constexpr float min4(float a, float b, float c, float d) {
    return std::min(std::min(a,b),std::min(c,d));
  }

  [[nodiscard]] constexpr float max4(float a, float b, float c, float d) {
    return std::max(std::max(a,b),std::max(c,d));
  }

  [[nodiscard]] constexpr float triangleArea(Vec2 a, Vec2 b, Vec2 c) {
    // triangle_area = len(cross(B-A,C-A)) / 2
    const Vec2 ba = b-a, ca = c-a;
    return abs((ba.x * ca.y) - (ba.y * ca.x)); // * .5f
  }

  bool barycentricCoord(Vec2 pt, Vec2 A, Vec2 B, Vec2 C,
                        float& u, float& v, float& w)
  {
    // P = uA + vB + wC
    // u = triangle_BCP_area / triangle_ABC_area
    // v = triangle_CAP_area / triangle_ABC_area
    // w = triangle_ABP_area / triangle_ABC_area
    //     (if inside triangle, w = 1 - u - v)

    // A-B  u-v
    // |/   |/
    // C    w
    const float area = triangleArea(A,B,C);
    u = triangleArea(pt, B, C) / area;
    v = triangleArea(A, pt, C) / area;
    w = triangleArea(A, B, pt) / area;
    return isOne(u + v + w);
  }

  inline void fixAngles(float& startAngle, float& endAngle) {
    while (endAngle <= startAngle) { endAngle += 360.0f; }
    endAngle = std::min(endAngle, startAngle + 360.0f);
  }
}

void DrawContext::line(Vec2 a, Vec2 b)
{
  if ((_color0 | _color1) == 0) { return; }
  if (_colorMode == CM_SOLID) {
    setColor();
    add(CMD_line2, a.x, a.y, b.x, b.y);
  } else {
    add(CMD_line2C,
        a.x, a.y, pointColor(a),
        b.x, b.y, pointColor(b));
  }
}

void DrawContext::line(const Vec3& a, const Vec3& b)
{
  if ((_color0 | _color1) == 0) { return; }
  if (_colorMode == CM_SOLID) {
    setColor();
    add(CMD_line3, a.x, a.y, a.z, b.x, b.y, b.z);
  } else {
    add(CMD_line3C,
        a.x, a.y, a.z, pointColor(a),
        b.x, b.y, b.z, pointColor(b));
  }
}

void DrawContext::triangle(Vec2 a, Vec2 b, Vec2 c)
{
  if ((_color0 | _color1) == 0) { return; }
  if (_colorMode == CM_SOLID) {
    setColor();
    add(CMD_triangle2, a.x, a.y, b.x, b.y, c.x, c.y);
  } else {
    add(CMD_triangle2C,
        a.x, a.y, pointColor(a),
        b.x, b.y, pointColor(b),
        c.x, c.y, pointColor(c));
  }
}

void DrawContext::triangle(const Vec3& a, const Vec3& b, const Vec3& c)
{
  if ((_color0 | _color1) == 0) { return; }
  if (_colorMode == CM_SOLID) {
    setColor();
    add(CMD_triangle3, a.x, a.y, a.z, b.x, b.y, b.z, c.x, c.y, c.z);
  } else {
    add(CMD_triangle3C,
        a.x, a.y, a.z, pointColor(a),
        b.x, b.y, b.z, pointColor(b),
        c.x, c.y, c.z, pointColor(c));
  }
}

void DrawContext::quad(
  const Vec2& a, const Vec2& b, const Vec2& c, const Vec2& d)
{
  if ((_color0 | _color1) == 0) { return; }
  if (_colorMode == CM_SOLID) {
    setColor();
    _quad(a,b,c,d);
  } else {
    add(CMD_quad2C,
        a.x, a.y, pointColor(a),
        b.x, b.y, pointColor(b),
        c.x, c.y, pointColor(c),
        d.x, d.y, pointColor(d));
  }
}

void DrawContext::quad(
  const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d)
{
  if ((_color0 | _color1) == 0) { return; }
  if (_colorMode == CM_SOLID) {
    setColor();
    add(CMD_quad3, a.x, a.y, a.z, b.x, b.y, b.z,
        c.x, c.y, c.z, d.x, d.y, d.z);
  } else {
    add(CMD_quad3C,
        a.x, a.y, a.z, pointColor(a),
        b.x, b.y, b.z, pointColor(b),
        c.x, c.y, c.z, pointColor(c),
        d.x, d.y, d.z, pointColor(d));
  }
}

void DrawContext::rectangle(const Rect& r)
{
  if (checkColor()) { _rectangle(r.x, r.y, r.w, r.h); }
}

void DrawContext::_rectangle(float x, float y, float w, float h)
{
  const float x1 = x + w;
  const float y1 = y + h;
  switch (_colorMode) {
    case CM_HGRADIENT: {
      const RGBA8 c0 = gradientColor(x);
      const RGBA8 c1 = gradientColor(x1);
      add(CMD_quad2C, x, y, c0, x1, y, c1, x, y1, c0, x1, y1, c1);
      break;
    }
    case CM_VGRADIENT: {
      const RGBA8 c0 = gradientColor(y);
      const RGBA8 c1 = gradientColor(y1);
      add(CMD_quad2C, x, y, c0, x1, y, c0, x, y1, c1, x1, y1, c1);
      break;
    }
    default:
      add(CMD_rectangle, x, y, x1, y1);
      break;
  }
}

void DrawContext::rectangle(const Rect& r, Vec2 t0, Vec2 t1)
{
  if ((_color0 | _color1) == 0) { return; }

  const float x0 = r.x;
  const float y0 = r.y;
  const float x1 = x0 + r.w;
  const float y1 = y0 + r.h;
  switch (_colorMode) {
    case CM_HGRADIENT: {
      const RGBA8 c0 = gradientColor(x0);
      const RGBA8 c1 = gradientColor(x1);
      add(CMD_quad2TC,
          x0, y0, t0.x, t0.y, c0,
          x1, y0, t1.x, t0.y, c1,
          x0, y1, t0.x, t1.y, c0,
          x1, y1, t1.x, t1.y, c1);
      break;
    }
    case CM_VGRADIENT: {
      const RGBA8 c0 = gradientColor(y0);
      const RGBA8 c1 = gradientColor(y1);
      add(CMD_quad2TC,
          x0, y0, t0.x, t0.y, c0,
          x1, y0, t1.x, t0.y, c0,
          x0, y1, t0.x, t1.y, c1,
          x1, y1, t1.x, t1.y, c1);
      break;
    }
    default:
      setColor();
      add(CMD_rectangleT, x0, y0, t0.x, t0.y, x1, y1, t1.x, t1.y);
      break;
  }
}

void DrawContext::rectangle(
  const Rect& r, Vec2 t0, Vec2 t1, const Rect& clip)
{
  if ((_color0 | _color1) == 0) { return; }

  float x0 = r.x;
  float y0 = r.y;
  float x1 = x0 + r.w;
  float y1 = y0 + r.h;
  const float cx0 = clip.x;
  const float cy0 = clip.y;
  const float cx1 = cx0 + clip.w;
  const float cy1 = cy0 + clip.h;

  if (x0 >= cx1 || y0 >= cy1 || x1 <= cx0 || y1 <= cy0) {
    return; // completely outside of clip region
  }

  float tx0 = t0.x;
  float tx1 = t1.x;
  if (x0 < cx0) { // left edge clipped
    tx0 += (tx1 - tx0) * ((cx0 - x0) / (x1 - x0));
    x0 = cx0;
  }
  if (x1 > cx1) { // right edge clipped
    tx1 -= (tx1 - tx0) * ((x1 - cx1) / (x1 - x0));
    x1 = cx1;
  }

  float ty0 = t0.y;
  float ty1 = t1.y;
  if (y0 < cy0) { // top clipped
    ty0 += (ty1 - ty0) * ((cy0 - y0) / (y1 - y0));
    y0 = cy0;
  }
  if (y1 > cy1) { // bottom clipped
    ty1 -= (ty1 - ty0) * ((y1 - cy1) / (y1 - y0));
    y1 = cy1;
  }

  if (_colorMode == CM_SOLID) {
    setColor();
    add(CMD_rectangleT, x0, y0, tx0, ty0, x1, y1, tx1, ty1);
  } else {
    add(CMD_quad2TC,
        x0, y0, tx0, ty0, pointColor({x0,y0}),
        x1, y0, tx1, ty0, pointColor({x1,y0}),
        x0, y1, tx0, ty1, pointColor({x0,y1}),
        x1, y1, tx1, ty1, pointColor({x1,y1}));
  }
}

void DrawContext::glyph(
  const TextFormatting& tf, Vec2 pos, AlignEnum align, int code)
{
  if (!checkColor()) { return; }

  GX_ASSERT(tf.font != nullptr);
  const Font& f = *tf.font;
  const Glyph* g = f.findGlyph(code);
  if (!g) {
    g = f.findGlyph(f.unknownCode());
    GX_ASSERT(g != nullptr);
  }

  if (!g->bitmap) { return; }

  const AlignEnum v_align = VAlign(align);
  if (v_align == ALIGN_TOP) {
    pos += tf.advY * f.ymax();
  } else {
    const float fs = float(f.size()) + tf.lineSpacing;
    if (v_align == ALIGN_BOTTOM) {
      pos += tf.advY * (f.ymin() - fs);
    } else { // ALIGN_VCENTER
      pos += tf.advY * ((f.ymax() - fs) * .5f);
    }
  }

  const AlignEnum h_align = HAlign(align);
  if (h_align != ALIGN_LEFT) {
    const float tw = f.glyphWidth(code);
    pos -= tf.advX * ((h_align == ALIGN_RIGHT) ? tw : (tw * .5f));
  }

  texture(f.tex());
  _glyph(*g, tf, pos, nullptr);
}

void DrawContext::_text(
  const TextFormatting& tf, Vec2 pos, AlignEnum align,
  std::string_view text, const Rect* clipPtr)
{
  if (text.empty() || !checkColor()) { return; }

  GX_ASSERT(tf.font != nullptr);
  const Font& f = *tf.font;
  const float fs = float(f.size()) + tf.lineSpacing;
  const AlignEnum h_align = HAlign(align);
  const AlignEnum v_align = VAlign(align);

  if (v_align == ALIGN_TOP) {
    pos += tf.advY * f.ymax();
  } else {
    int nl = 0;
    for (int ch : text) { nl += (ch == '\n'); }
    if (v_align == ALIGN_BOTTOM) {
      pos += tf.advY * (f.ymin() - (fs*float(nl)));
    } else { // ALIGN_VCENTER
      pos += tf.advY * ((f.ymax() - (fs*float(nl))) * .5f);
    }
  }

  texture(f.tex());
  std::size_t lineStart = 0;
  Vec2 startPos = pos;

  for (;;) {
    const std::size_t i = text.find('\n', lineStart);
    const std::string_view line = text.substr(
      lineStart, (i != std::string_view::npos) ? (i - lineStart) : i);

    if (!line.empty()) {
      if (h_align != ALIGN_LEFT) {
        const float tw = f.calcLength(line, tf.glyphSpacing);
        pos -= tf.advX * ((h_align == ALIGN_RIGHT) ? tw : (tw * .5f));
      }

      for (UTF8Iterator itr{line}; !itr.done(); itr.next()) {
        int ch = itr.get();
        if (ch == '\t') { ch = ' '; }
        const Glyph* g = f.findGlyph(ch);
        if (!g) {
          g = f.findGlyph(f.unknownCode());
          GX_ASSERT(g != nullptr);
        }

        if (g->bitmap) { _glyph(*g, tf, pos, clipPtr); }
        pos += tf.advX * (g->advX + tf.glyphSpacing);
      }
    }

    if (i == std::string_view::npos) { break; }

    // move to start of next line
    lineStart = i + 1;
    startPos += tf.advY * fs;
    pos = startPos;
  }
}

void DrawContext::_glyph(
  const Glyph& g, const TextFormatting& tf, Vec2 pos, const Rect* clipPtr)
{
  const Vec2 gx = tf.glyphX * float(g.width);
  const Vec2 gy = tf.glyphY * float(g.height);

  // quad: A-B
  //       |/|
  //       C-D

  Vec2 A = pos + (tf.glyphX * g.left) - (tf.glyphY * g.top);
  Vec2 B = A + gx;
  Vec2 C = A + gy;
  Vec2 D = C + gx;

  Vec2 At{g.t0.x, g.t0.y};
  Vec2 Bt{g.t1.x, g.t0.y};
  Vec2 Ct{g.t0.x, g.t1.y};
  Vec2 Dt{g.t1.x, g.t1.y};

  if (clipPtr) {
    const float cx0 = clipPtr->x;
    const float cy0 = clipPtr->y;
    const float cx1 = cx0 + clipPtr->w;
    const float cy1 = cy0 + clipPtr->h;

    // discard check
    if (max4(A.x, B.x, C.x, D.x) <= cx0
        || min4(A.x, B.x, C.x, D.x) >= cx1
        || max4(A.y, B.y, C.y, D.y) <= cy0
        || min4(A.y, B.y, C.y, D.y) >= cy1) { return; }

    // (overly) simplified clipping of quad
    // note that too much can be clipped if quad is not at a
    // 0/90/180/270 degree rotation since no new triangles are
    // created as part of the clipping

    const Vec2 newA{std::clamp(A.x,cx0,cx1), std::clamp(A.y,cy0,cy1)};
    const Vec2 newB{std::clamp(B.x,cx0,cx1), std::clamp(B.y,cy0,cy1)};
    const Vec2 newC{std::clamp(C.x,cx0,cx1), std::clamp(C.y,cy0,cy1)};
    const Vec2 newD{std::clamp(D.x,cx0,cx1), std::clamp(D.y,cy0,cy1)};

    // use barycentric coordinates to update texture coords
    Vec2 newAt = At, newBt = Bt, newCt = Ct, newDt = Dt;

    if (A != newA) {
      // A-B   u-v .   B     w .
      // |/    |/  .  /|    /| .
      // C     w   . C-D   v-u .
      float u, v, w;
      if (barycentricCoord(newA, A, B, C, u, v, w)) {
        newAt = At*u + Bt*v + Ct*w;
      } else if (barycentricCoord(newA, D, C, B, u, v, w)) {
        newAt = Dt*u + Ct*v + Bt*w;
      }
    }

    if (B != newB) {
      // A-B   w-u . A     v   .
      //  \|    \| . |\    |\  .
      //   D     v . C-D   u-w .
      float u, v, w;
      if (barycentricCoord(newB, B, D, A, u, v, w)) {
        newBt = Bt*u + Dt*v + At*w;
      } else if (barycentricCoord(newB, C, A, D, u, v, w)) {
        newBt = Ct*u + At*v + Dt*w;
      }
    }

    if (C != newC) {
      // A     v   . A-B   w-u .
      // |\    |\  .  \|    \| .
      // C-D   u-w .   D     v .
      float u, v, w;
      if (barycentricCoord(newC, C, A, D, u, v, w)) {
        newCt = Ct*u + At*v + Dt*w;
      } else if (barycentricCoord(newC, B, D, A, u, v, w)) {
        newCt = Bt*u + Dt*v + At*w;
      }
    }

    if (D != newD) {
      //   B     w . A-B   u-v .
      //  /|    /| . |/    |/  .
      // C-D   v-u . C     w   .
      float u, v, w;
      if (barycentricCoord(newD, D, C, B, u, v, w)) {
        newDt = Dt*u + Ct*v + Bt*w;
      } else if (barycentricCoord(newD, A, B, C, u, v, w)) {
        newDt = At*u + Bt*v + Ct*w;
      }
    }

    A = newA; B = newB; C = newC; D = newD;
    At = newAt; Bt = newBt; Ct = newCt; Dt = newDt;
  }

  if (_colorMode == CM_SOLID) {
    add(CMD_quad2T,
        A.x, A.y, At.x, At.y,
        B.x, B.y, Bt.x, Bt.y,
        C.x, C.y, Ct.x, Ct.y,
        D.x, D.y, Dt.x, Dt.y);
  } else {
    add(CMD_quad2TC,
        A.x, A.y, At.x, At.y, pointColor(A),
        B.x, B.y, Bt.x, Bt.y, pointColor(B),
        C.x, C.y, Ct.x, Ct.y, pointColor(C),
        D.x, D.y, Dt.x, Dt.y, pointColor(D));
  }
}

void DrawContext::circleSector(
  Vec2 center, float radius, float startAngle, float endAngle, int segments)
{
  if (!checkColor()) { return; }

  fixAngles(startAngle, endAngle);
  _circleSector(
    center, radius, degToRad(startAngle), degToRad(endAngle), segments);
}

void DrawContext::_circleSector(
  Vec2 center, float radius, float angle0, float angle1, int segments)
{
  const float segmentAngle = (angle1 - angle0) / float(segments);
  const Vec2 v0{center.x, center.y};

  Vec2 v1{
    center.x + (radius * std::sin(angle0)),
    center.y - (radius * std::cos(angle0))};

  float a = angle0;
  for (int i = 0; i < segments; ++i) {
    if (i == segments-1) { a = angle1; } else { a += segmentAngle; }

    const Vec2 v2{
      center.x + (radius * std::sin(a)),
      center.y - (radius * std::cos(a))};

    if (_colorMode == CM_SOLID) {
      add(CMD_triangle2, v0.x, v0.y, v1.x, v1.y, v2.x, v2.y);
    } else {
      add(CMD_triangle2C,
          v0.x, v0.y, pointColor(v0),
          v1.x, v1.y, pointColor(v1),
          v2.x, v2.y, pointColor(v2));
    }

    // setup for next iteration
    v1 = v2;
  }
}

void DrawContext::circleSector(
  Vec2 center, float radius, float startAngle, float endAngle, int segments,
  RGBA8 innerColor, RGBA8 outerColor)
{
  if ((innerColor | outerColor) == 0) { return; }

  fixAngles(startAngle, endAngle);
  const float angle0 = degToRad(startAngle);
  const float angle1 = degToRad(endAngle);
  const float segmentAngle = (angle1 - angle0) / float(segments);

  const Vertex2C v0{center, innerColor};

  Vertex2C v1{
    center.x + (radius * std::sin(angle0)),
    center.y - (radius * std::cos(angle0)),
    outerColor};

  Vertex2C v2;
  v2.c = outerColor;

  float a = angle0;
  for (int i = 0; i < segments; ++i) {
    if (i == segments-1) { a = angle1; } else { a += segmentAngle; }

    v2.x = center.x + (radius * std::sin(a));
    v2.y = center.y - (radius * std::cos(a));

    triangle(v0, v1, v2);

    // setup for next iteration
    v1.x = v2.x;
    v1.y = v2.y;
  }
}

void DrawContext::arc(
  Vec2 center, float radius, float startAngle, float endAngle,
  int segments, float arcWidth)
{
  if (!checkColor()) { return; }

  fixAngles(startAngle, endAngle);
  _arc(center, radius, degToRad(startAngle), degToRad(endAngle),
       segments, arcWidth);
}

void DrawContext::_arc(
  Vec2 center, float radius, float angle0, float angle1,
  int segments, float arcWidth)
{
  const float segmentAngle = (angle1 - angle0) / float(segments);
  const float innerR = radius - arcWidth;

  const float sa0 = std::sin(angle0), ca0 = std::cos(angle0);
  Vec2 v0{center.x + (radius * sa0), center.y - (radius * ca0)};
  Vec2 v1{center.x + (innerR * sa0), center.y - (innerR * ca0)};

  float a = angle0;
  for (int i = 0; i < segments; ++i) {
    if (i == segments-1) { a = angle1; } else { a += segmentAngle; }

    const float sa = std::sin(a), ca = std::cos(a);
    const Vec2 v2{center.x + (radius * sa), center.y - (radius * ca)};
    const Vec2 v3{center.x + (innerR * sa), center.y - (innerR * ca)};

    if (_colorMode == CM_SOLID) {
      _quad(v0, v1, v2, v3);
    } else {
      add(CMD_quad2C,
          v0.x, v0.y, pointColor(v0),
          v1.x, v1.y, pointColor(v1),
          v2.x, v2.y, pointColor(v2),
          v3.x, v3.y, pointColor(v3));
    }

    // setup for next iteration
    v0 = v2;
    v1 = v3;
  }
}

void DrawContext::_arc(
  Vec2 center, float radius, float angle0, float angle1, int segments,
  float arcWidth, RGBA8 innerColor, RGBA8 outerColor, RGBA8 fillColor)
{
  const float segmentAngle = (angle1 - angle0) / float(segments);
  const float innerR = radius - arcWidth;

  const float sa0 = std::sin(angle0), ca0 = std::cos(angle0);
  Vec2 v0{center.x + (radius * sa0), center.y - (radius * ca0)};
  Vec2 v1{center.x + (innerR * sa0), center.y - (innerR * ca0)};

  float a = angle0;
  for (int i = 0; i < segments; ++i) {
    if (i == segments-1) { a = angle1; } else { a += segmentAngle; }

    const float sa = std::sin(a), ca = std::cos(a);
    const Vec2 v2{center.x + (radius * sa), center.y - (radius * ca)};
    const Vec2 v3{center.x + (innerR * sa), center.y - (innerR * ca)};

    if (innerColor | outerColor) {
      add(CMD_quad2C,
          v0.x, v0.y, outerColor,
          v1.x, v1.y, innerColor,
          v2.x, v2.y, outerColor,
          v3.x, v3.y, innerColor);
    }

    if (fillColor) {
      add(CMD_triangle2C,
          v1.x, v1.y, fillColor,
          v3.x, v3.y, fillColor,
          center.x, center.y, fillColor);
    }

    // setup for next iteration
    v0 = v2;
    v1 = v3;
  }
}

void DrawContext::arc(
  Vec2 center, float radius, float startAngle, float endAngle,
  int segments, float arcWidth, RGBA8 startColor, RGBA8 endColor)
{
  if ((startColor | endColor) == 0) { return; }

  fixAngles(startAngle, endAngle);
  const float angle0 = degToRad(startAngle);
  const float angle1 = degToRad(endAngle);
  const float segmentAngle = (angle1 - angle0) / float(segments);
  const float innerR = radius - arcWidth;

  const float sa0 = std::sin(angle0), ca0 = std::cos(angle0);
  Vertex2C v0{center.x + (radius * sa0), center.y - (radius * ca0), startColor};
  Vertex2C v1{center.x + (innerR * sa0), center.y - (innerR * ca0), startColor};

  const Color full0 = unpackRGBA8(startColor);
  const Color full1 = unpackRGBA8(endColor);

  float a = angle0;
  for (int i = 0; i < segments; ++i) {
    RGBA8 c;
    if (i == segments-1) {
      a = angle1;
      c = endColor;
    } else {
      a += segmentAngle;
      const float x = float(i+1) / float(segments);
      c = packRGBA8((full0 * (1.0f-x)) + (full1 * x));
    }

    const float sa = std::sin(a), ca = std::cos(a);
    const Vertex2C v2{center.x + (radius * sa), center.y - (radius * ca), c};
    const Vertex2C v3{center.x + (innerR * sa), center.y - (innerR * ca), c};

    quad(v0, v1, v2, v3);

    // setup for next iteration
    v0 = v2;
    v1 = v3;
  }
}

void DrawContext::roundedRectangle(
  float x, float y, float w, float h, float curveRadius, int curveSegments)
{
  if (!checkColor()) { return; }

  const float half_w = w * .5f;
  const float half_h = h * .5f;
  const float r = std::min(curveRadius, std::min(half_w, half_h));

  // corners
  constexpr float a90  = degToRad(90.0f);
  constexpr float a180 = degToRad(180.0f);
  constexpr float a270 = degToRad(270.0f);
  constexpr float a360 = degToRad(360.0f);
  _circleSector({x+r,y+r}, r, a270, a360, curveSegments);    // top/left
  _circleSector({x+w-r,y+r}, r, 0, a90, curveSegments);      // top/right
  _circleSector({x+w-r,y+h-r}, r, a90, a180, curveSegments); // bottom/right
  _circleSector({x+r,y+h-r}, r, a180, a270, curveSegments);  // bottom/left

  // borders/center
  const float rr = r * 2.0f;
  if (r < half_w) {
    // fill top/bottom borders & center
    _rectangle(x+r, y, w-rr, h);
    if (r < half_h) {
      // fill left border
      _rectangle(x, y+r, r, h-rr);
      // fill right border
      _rectangle(x+w-r, y+r, r, h-rr);
    }
  } else if (r < half_h) {
    // fill left/right borders
    _rectangle(x, y+r, w, h-rr);
  }
}

void DrawContext::border(float x, float y, float w, float h, float borderWidth)
{
  if ((_color0 | _color1) == 0) { return; }

  const Vec2 A{x,y};
  const Vec2 B{x+w,y};
  const Vec2 C{x,y+h};
  const Vec2 D{x+w,y+h};

  const Vec2 iA{x+borderWidth,y+borderWidth};
  const Vec2 iB{x+w-borderWidth,y+borderWidth};
  const Vec2 iC{x+borderWidth,y+h-borderWidth};
  const Vec2 iD{x+w-borderWidth,y+h-borderWidth};

  if (_colorMode == CM_SOLID) {
    setColor();
    _quad(A,B,iA,iB); // top
    _quad(iC,iD,C,D); // bottom
    _quad(A,iA,C,iC); // left
    _quad(iB,B,iD,D); // right
  } else {
    const Vertex2C v_A{A, pointColor(A)};
    const Vertex2C v_B{B, pointColor(B)};
    const Vertex2C v_C{C, pointColor(C)};
    const Vertex2C v_D{D, pointColor(D)};

    const Vertex2C v_iA{iA, pointColor(iA)};
    const Vertex2C v_iB{iB, pointColor(iB)};
    const Vertex2C v_iC{iC, pointColor(iC)};
    const Vertex2C v_iD{iD, pointColor(iD)};

    quad(v_A, v_B, v_iA, v_iB);
    quad(v_iC, v_iD, v_C, v_D);
    quad(v_A, v_iA, v_C, v_iC);
    quad(v_iB, v_B, v_iD, v_D);
  }
}

void DrawContext::border(float x, float y, float w, float h, float borderWidth,
                         RGBA8 innerColor, RGBA8 outerColor, RGBA8 fillColor)
{
  if ((innerColor | outerColor | fillColor) == 0) { return; }

  float x0 = x, y0 = y, x1 = x+w, y1 = y+h;
  const Vertex2C v_A{x0, y0, outerColor};
  const Vertex2C v_B{x1, y0, outerColor};
  const Vertex2C v_C{x0, y1, outerColor};
  const Vertex2C v_D{x1, y1, outerColor};

  x0 += borderWidth; y0 += borderWidth;
  x1 -= borderWidth; y1 -= borderWidth;
  Vertex2C v_iA{x0, y0, innerColor};
  Vertex2C v_iB{x1, y0, innerColor};
  Vertex2C v_iC{x0, y1, innerColor};
  Vertex2C v_iD{x1, y1, innerColor};

  if (innerColor | outerColor) {
    quad(v_A, v_B, v_iA, v_iB);
    quad(v_iC, v_iD, v_C, v_D);
    quad(v_A, v_iA, v_C, v_iC);
    quad(v_iB, v_B, v_iD, v_D);
  }

  if (fillColor) {
    v_iA.c = fillColor;
    v_iB.c = fillColor;
    v_iC.c = fillColor;
    v_iD.c = fillColor;
    quad(v_iA, v_iB, v_iC, v_iD);
  }
}

void DrawContext::roundedBorder(
  float x, float y, float w, float h,
  float curveRadius, int curveSegments, float borderWidth)
{
  if (!checkColor()) { return; }

  const float half_w = w * .5f;
  const float half_h = h * .5f;
  const float r = std::min(curveRadius, std::min(half_w, half_h));

  // corners
  constexpr float a90  = degToRad(90.0f);
  constexpr float a180 = degToRad(180.0f);
  constexpr float a270 = degToRad(270.0f);
  constexpr float a360 = degToRad(360.0f);
  _arc({x+r,y+r}, r, a270, a360, curveSegments, borderWidth);    // top/left
  _arc({x+w-r,y+r}, r, 0, a90, curveSegments, borderWidth);      // top/right
  _arc({x+w-r,y+h-r}, r, a90, a180, curveSegments, borderWidth); // bottom/right
  _arc({x+r,y+h-r}, r, a180, a270, curveSegments, borderWidth);  // bottom/left

  // borders/center
  if (r < half_w) {
    // top/bottom borders
    const float bw = w - (r * 2.0f);
    _rectangle(x+r, y, bw, borderWidth);
    _rectangle(x+r, y+h-borderWidth, bw, borderWidth);
  }

  if (r < half_h) {
    // left/right borders
    const float bh = h - (r * 2.0f);
    _rectangle(x, y+r, borderWidth, bh);
    _rectangle(x+w-borderWidth, y+r, borderWidth, bh);
  }
}

void DrawContext::roundedBorder(
  float x, float y, float w, float h,
  float curveRadius, int curveSegments, float borderWidth,
  RGBA8 innerColor, RGBA8 outerColor, RGBA8 fillColor)
{
  if ((innerColor | outerColor | fillColor) == 0) { return; }

  const float half_w = w * .5f;
  const float half_h = h * .5f;
  const float r = std::min(curveRadius, std::min(half_w, half_h));

  // corners
  constexpr float a90  = degToRad(90.0f);
  constexpr float a180 = degToRad(180.0f);
  constexpr float a270 = degToRad(270.0f);
  constexpr float a360 = degToRad(360.0f);
  _arc({x+r,y+r}, r, a270, a360, curveSegments, borderWidth,
       innerColor, outerColor, fillColor); // top/left
  _arc({x+w-r,y+r}, r, 0, a90, curveSegments, borderWidth,
       innerColor, outerColor, fillColor); // top/right
  _arc({x+w-r,y+h-r}, r, a90, a180, curveSegments, borderWidth,
       innerColor, outerColor, fillColor); // bottom/right
  _arc({x+r,y+h-r}, r, a180, a270, curveSegments, borderWidth,
       innerColor, outerColor, fillColor); // bottom/left

  // borders/center
  if (r < half_w) {
    // top/bottom borders
    const float x0 = x + r;
    const float x1 = x + w - r;
    add(CMD_quad2C,
        x0, y,               outerColor,
        x1, y,               outerColor,
        x0, y+borderWidth,   innerColor,
        x1, y+borderWidth,   innerColor);
    add(CMD_quad2C,
        x0, y+h-borderWidth, innerColor,
        x1, y+h-borderWidth, innerColor,
        x0, y+h,             outerColor,
        x1, y+h,             outerColor);
  }

  if (r < half_h) {
    // left/right borders
    const float y0 = y + r;
    const float y1 = y + h - r;
    add(CMD_quad2C,
        x,               y0, outerColor,
        x+borderWidth,   y0, innerColor,
        x,               y1, outerColor,
        x+borderWidth,   y1, innerColor);
    add(CMD_quad2C,
        x+w-borderWidth, y0, innerColor,
        x+w,             y0, outerColor,
        x+w-borderWidth, y1, innerColor,
        x+w,             y1, outerColor);
  }

  if (fillColor) {
    if (r < half_w) {
      // fill top/bottom borders & center
      add(CMD_quad2C,
          x+r,   y+borderWidth,   fillColor,
          x+w-r, y+borderWidth,   fillColor,
          x+r,   y+h-borderWidth, fillColor,
          x+w-r, y+h-borderWidth, fillColor);
      if (r < half_h) {
        const float y0 = y + r;
        const float y1 = y + h - r;

        // fill left border
        add(CMD_quad2C,
            x+borderWidth,   y0, fillColor,
            x+r,             y0, fillColor,
            x+borderWidth,   y1, fillColor,
            x+r,             y1, fillColor);
        // fill right border
        add(CMD_quad2C,
            x+w-r,           y0, fillColor,
            x+w-borderWidth, y0, fillColor,
            x+w-r,           y1, fillColor,
            x+w-borderWidth, y1, fillColor);
      }
    } else if (r < half_h) {
      // fill left/right borders & center
      add(CMD_quad2C,
          x,   y+r,   fillColor,
          x+w, y+r,   fillColor,
          x,   y+h-r, fillColor,
          x+w, y+h-r, fillColor);
    }
  }
}
