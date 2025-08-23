//
// gx/DrawContext.cc
// Copyright (C) 2025 Richard Bradley
//

#include "DrawContext.hh"
#include "Font.hh"
#include "TextFormat.hh"
#include "Unicode.hh"
#include "MathUtil.hh"
#include "Assert.hh"
using namespace gx;


namespace {
  template<class T>
  [[nodiscard]] constexpr T min3(T a, T b, T c) {
    return std::min(a, std::min(b, c));
  }

  template<class T>
  [[nodiscard]] constexpr T min4(T a, T b, T c, T d) {
    return std::min(std::min(a, b), std::min(c, d));
  }

  template<class T>
  [[nodiscard]] constexpr T max4(T a, T b, T c, T d) {
    return std::max(std::max(a, b), std::max(c, d));
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


DrawContext::DrawContext(DrawList* dl) : _dl{dl}
{
  GX_ASSERT(_dl != nullptr);
  init();
}

void DrawContext::line(Vec2 a, Vec2 b)
{
  if ((_color0 | _color1) == 0) { return; }
  if (_colorMode == CM_SOLID) {
    setColor();
    _dl->line2(a, b);
  } else {
    _dl->line2C({a.x, a.y, pointColor(a)},
                {b.x, b.y, pointColor(b)});
  }
}

void DrawContext::line(const Vec3& a, const Vec3& b)
{
  if ((_color0 | _color1) == 0) { return; }
  if (_colorMode == CM_SOLID) {
    setColor();
    _dl->line3(a, b);
  } else {
    _dl->line3C({a.x, a.y, a.z, pointColor(a)},
                {b.x, b.y, b.z, pointColor(b)});
  }
}

void DrawContext::lineStart(Vec2 a)
{
  if ((_color0 | _color1) == 0) { return; }
  if (_colorMode == CM_SOLID) {
    setColor();
    _dl->lineStart2(a);
  } else {
    _dl->lineStart2C({a.x, a.y, pointColor(a)});
  }
}

void DrawContext::lineTo(Vec2 a)
{
  if ((_color0 | _color1) == 0) { return; }
  if (_colorMode == CM_SOLID) {
    setColor();
    _dl->lineTo2(a);
  } else {
    _dl->lineTo2C({a.x, a.y, pointColor(a)});
  }
}

void DrawContext::lineStart(const Vec3& a)
{
  if ((_color0 | _color1) == 0) { return; }
  if (_colorMode == CM_SOLID) {
    setColor();
    _dl->lineStart3(a);
  } else {
    _dl->lineStart3C({a.x, a.y, a.z, pointColor(a)});
  }
}

void DrawContext::lineTo(const Vec3& a)
{
  if ((_color0 | _color1) == 0) { return; }
  if (_colorMode == CM_SOLID) {
    setColor();
    _dl->lineTo3(a);
  } else {
    _dl->lineTo3C({a.x, a.y, a.z, pointColor(a)});
  }
}

void DrawContext::triangle(Vec2 a, Vec2 b, Vec2 c)
{
  if ((_color0 | _color1) == 0) { return; }
  if (_colorMode == CM_SOLID) {
    setColor();
    _dl->triangle2(a, b, c);
  } else {
    _dl->triangle2C({a.x, a.y, pointColor(a)},
                    {b.x, b.y, pointColor(b)},
                    {c.x, c.y, pointColor(c)});
  }
}

void DrawContext::triangle(const Vec3& a, const Vec3& b, const Vec3& c)
{
  if ((_color0 | _color1) == 0) { return; }
  if (_colorMode == CM_SOLID) {
    setColor();
    _dl->triangle3(a, b, c);
  } else {
    _dl->triangle3C({a.x, a.y, a.z, pointColor(a)},
                    {b.x, b.y, b.z, pointColor(b)},
                    {c.x, c.y, c.z, pointColor(c)});
  }
}

void DrawContext::quad(
  const Vec2& a, const Vec2& b, const Vec2& c, const Vec2& d)
{
  if ((_color0 | _color1) == 0) { return; }
  if (_colorMode == CM_SOLID) {
    setColor();
    _dl->quad2(a, b, c, d);
  } else {
    _dl->quad2C({a.x, a.y, pointColor(a)},
                {b.x, b.y, pointColor(b)},
                {c.x, c.y, pointColor(c)},
                {d.x, d.y, pointColor(d)});
  }
}

void DrawContext::quad(
  const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d)
{
  if ((_color0 | _color1) == 0) { return; }
  if (_colorMode == CM_SOLID) {
    setColor();
    _dl->quad3(a, b, c, d);
  } else {
    _dl->quad3C({a.x, a.y, a.z, pointColor(a)},
                {b.x, b.y, b.z, pointColor(b)},
                {c.x, c.y, c.z, pointColor(c)},
                {d.x, d.y, d.z, pointColor(d)});
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
      _dl->quad2C({x, y, c0}, {x1, y, c1}, {x, y1, c0}, {x1, y1, c1});
      break;
    }
    case CM_VGRADIENT: {
      const RGBA8 c0 = gradientColor(y);
      const RGBA8 c1 = gradientColor(y1);
      _dl->quad2C({x, y, c0}, {x1, y, c0}, {x, y1, c1}, {x1, y1, c1});
      break;
    }
    default:
      _dl->rectangle({x, y}, {x1, y1});
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
      _dl->quad2TC({x0, y0, t0.x, t0.y, c0},
                   {x1, y0, t1.x, t0.y, c1},
                   {x0, y1, t0.x, t1.y, c0},
                   {x1, y1, t1.x, t1.y, c1});
      break;
    }
    case CM_VGRADIENT: {
      const RGBA8 c0 = gradientColor(y0);
      const RGBA8 c1 = gradientColor(y1);
      _dl->quad2TC({x0, y0, t0.x, t0.y, c0},
                   {x1, y0, t1.x, t0.y, c0},
                   {x0, y1, t0.x, t1.y, c1},
                   {x1, y1, t1.x, t1.y, c1});
      break;
    }
    default:
      setColor();
      _dl->rectangleT({x0, y0, t0.x, t0.y}, {x1, y1, t1.x, t1.y});
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
    _dl->rectangleT({x0, y0, tx0, ty0}, {x1, y1, tx1, ty1});
  } else {
    _dl->quad2TC({x0, y0, tx0, ty0, pointColor({x0,y0})},
                 {x1, y0, tx1, ty0, pointColor({x1,y0})},
                 {x0, y1, tx0, ty1, pointColor({x0,y1})},
                 {x1, y1, tx1, ty1, pointColor({x1,y1})});
  }
}

void DrawContext::glyph(
  const TextFormat& tf, Vec2 pos, AlignEnum align, int code)
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
  } else if (v_align == ALIGN_BOTTOM) {
    const float lh = float(f.size()) + tf.lineSpacing;
    pos += tf.advY * (f.ymin() - lh);
  } else if (v_align == ALIGN_VCENTER) {
    const float lh = float(f.size()) + tf.lineSpacing;
    pos += tf.advY * ((f.ymax() - lh) * .5f);
  } // otherwise, pos is baseline

  const AlignEnum h_align = HAlign(align);
  if (h_align != ALIGN_LEFT) {
    const float tw = f.glyphWidth(code);
    pos -= tf.advX * ((h_align == ALIGN_RIGHT) ? tw : (tw * .5f));
  }

  texture(f.atlas());
  _glyph(*g, tf, pos, nullptr);
}

void DrawContext::_text(
  const TextFormat& tf, Vec2 pos, AlignEnum align,
  std::string_view text, const Rect* clipPtr)
{
  if (text.empty()) { return; }

  GX_ASSERT(tf.font != nullptr);
  const Font& f = *tf.font;
  const float lh = float(f.size()) + tf.lineSpacing;
  const AlignEnum h_align = HAlign(align);
  const AlignEnum v_align = VAlign(align);

  if (v_align == ALIGN_TOP) {
    pos += tf.advY * f.ymax();
  } else if (v_align == ALIGN_BOTTOM) {
    int nl = 0;
    for (int ch : text) { nl += (ch == '\n'); }
    pos += tf.advY * (f.ymin() - (lh*float(nl)));
  } else if (v_align == ALIGN_VCENTER) {
    int nl = 0;
    for (int ch : text) { nl += (ch == '\n'); }
    pos += tf.advY * ((f.ymax() - (lh*float(nl))) * .5f);
  } // otherwise, pos is baseline of 1st line

  texture(f.atlas());
  TextState ts;
  if (_colorMode == CM_SOLID) {
    setColor();
    ts.pushColor(_color0);
  }
  std::size_t lineStart = 0;

  Vec2 ulPos;
  float ulLen = 0;
  bool underline = false;

  for (;;) {
    const std::size_t i = text.find('\n', lineStart);
    const std::string_view line = text.substr(
      lineStart, (i != std::string_view::npos) ? (i - lineStart) : i);

    if (!line.empty()) {
      float offset = 0;
      if (h_align != ALIGN_LEFT) {
        const float tw = tf.calcLength(line);
        offset = (h_align == ALIGN_RIGHT) ? tw : (tw * .5f);
      }

      float len = 0;
      enum { UL_noop, UL_start, UL_end} ulOp = UL_noop;
      for (UTF8Iterator itr{line}; itr; ++itr) {
        if (ulOp == UL_end) {
          const Glyph* g = f.findGlyph('_');
          if (g) { _glyph(*g, tf, ulPos, clipPtr, ulLen - tf.glyphSpacing); }
          underline = false;
          ulOp = UL_noop;
        }

        int ch = *itr;
        if (ch == tf.startTag && tf.startTag != 0) {
          const std::size_t startPos = itr.pos() + 1;
          const std::size_t endPos = findUTF8(line, tf.endTag, startPos);
          if (endPos != std::string_view::npos) {
            const auto tag = line.substr(startPos, endPos - startPos);
            const auto tagType = tf.parseTag(ts, tag);
            if (tagType != TAG_unknown) {
              if (tagType == TAG_color) {
                const RGBA8 c = ts.color();
                if (c != 0 && (_colorMode != CM_SOLID || _color0 != c)) {
                  color(c);
                  setColor();
                }
              } else if (tagType == TAG_underline) {
                if (!underline && ts.underline()) {
                  ulOp = UL_start;
                } else if (underline && !ts.underline()) {
                  ulOp = UL_end;
                }
              }

              itr.setPos(endPos);
              continue;
            }
          }
        } else if (ch == '\t') {
          if (tf.tabWidth <= 0) {
            ch = ' ';
          } else {
            len = (std::floor(len / tf.tabWidth) + 1.0f) * tf.tabWidth;
            continue;
          }
        }

        const Glyph* g = f.findGlyph(ch);
        if (!g) {
          g = f.findGlyph(f.unknownCode());
          GX_ASSERT(g != nullptr);
        }

        const Vec2 p = pos + (tf.advX * (len - offset));
        if (g->bitmap) { _glyph(*g, tf, p, clipPtr); }
        len += g->advX + tf.glyphSpacing;

        if (ulOp == UL_start) {
          ulPos = p; ulLen = 0;
          underline = true; ulOp = UL_noop;
        }

        if (underline) {
          ulLen += g->advX + tf.glyphSpacing;
        }
      }

      // finish end-of-line underline
      if (underline) {
        const Glyph* ulg = f.findGlyph('_');
        if (ulg) { _glyph(*ulg, tf, ulPos, clipPtr, ulLen - tf.glyphSpacing); }
        underline = false;
        ulOp = (ulOp == UL_end) ? UL_noop : UL_start;
      }
    }

    if (i == std::string_view::npos) { break; }

    // move to start of next line
    lineStart = i + 1;
    pos += tf.advY * lh;
  }
}

void DrawContext::_glyph(
  const Glyph& g, const TextFormat& tf, Vec2 baseline,
  const Rect* clipPtr, float altWidth)
{
  const Vec2 gx = tf.glyphX * (altWidth > 0 ? altWidth : float(g.width));
  const Vec2 gy = tf.glyphY * float(g.height);

  // quad: A-B
  //       |/|
  //       C-D

  Vec2 A = baseline + (tf.glyphX * g.left) - (tf.glyphY * g.top);
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
    _dl->quad2T({A.x, A.y, At.x, At.y},
                {B.x, B.y, Bt.x, Bt.y},
                {C.x, C.y, Ct.x, Ct.y},
                {D.x, D.y, Dt.x, Dt.y});
  } else {
    _dl->quad2TC({A.x, A.y, At.x, At.y, pointColor(A)},
                 {B.x, B.y, Bt.x, Bt.y, pointColor(B)},
                 {C.x, C.y, Ct.x, Ct.y, pointColor(C)},
                 {D.x, D.y, Dt.x, Dt.y, pointColor(D)});
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
      _dl->triangle2(v0, v1, v2);
    } else {
      _dl->triangle2C({v0.x, v0.y, pointColor(v0)},
                      {v1.x, v1.y, pointColor(v1)},
                      {v2.x, v2.y, pointColor(v2)});
    }

    // setup for next iteration
    v1 = v2;
  }
}

void DrawContext::circleSectorShaded(
  Vec2 center, float radius, float startAngle, float endAngle, int segments,
  RGBA8 innerColor, RGBA8 outerColor)
{
  if ((innerColor | outerColor) == 0) { return; }

  fixAngles(startAngle, endAngle);
  const float angle0 = degToRad(startAngle);
  const float angle1 = degToRad(endAngle);
  const float segmentAngle = (angle1 - angle0) / float(segments);

  const Vertex2C v0{center.x, center.y, innerColor};

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
      _dl->quad2(v0, v1, v2, v3);
    } else {
      _dl->quad2C({v0.x, v0.y, pointColor(v0)},
                  {v1.x, v1.y, pointColor(v1)},
                  {v2.x, v2.y, pointColor(v2)},
                  {v3.x, v3.y, pointColor(v3)});
    }

    // setup for next iteration
    v0 = v2;
    v1 = v3;
  }
}

void DrawContext::_arcShaded(
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
      _dl->quad2C({v0.x, v0.y, outerColor},
                  {v1.x, v1.y, innerColor},
                  {v2.x, v2.y, outerColor},
                  {v3.x, v3.y, innerColor});
    }

    if (fillColor) {
      _dl->triangle2C({v1.x, v1.y, fillColor},
                      {v3.x, v3.y, fillColor},
                      {center.x, center.y, fillColor});
    }

    // setup for next iteration
    v0 = v2;
    v1 = v3;
  }
}

void DrawContext::arcShaded(
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
  const Rect& r, float curveRadius, int curveSegments)
{
  if (!checkColor()) { return; }

  const auto [x,y,w,h] = r;
  const float half_w = w * .5f;
  const float half_h = h * .5f;
  const float cr = min3(curveRadius, half_w, half_h);

  // corners
  constexpr float a90  = degToRad(90.0f);
  constexpr float a180 = degToRad(180.0f);
  constexpr float a270 = degToRad(270.0f);
  constexpr float a360 = degToRad(360.0f);
  _circleSector({x+cr,y+cr}, cr, a270, a360, curveSegments);    // top/left
  _circleSector({x+w-cr,y+cr}, cr, 0, a90, curveSegments);      // top/right
  _circleSector({x+w-cr,y+h-cr}, cr, a90, a180, curveSegments); // bottom/right
  _circleSector({x+cr,y+h-cr}, cr, a180, a270, curveSegments);  // bottom/left

  // borders/center
  const float cr2 = cr * 2.0f;
  if (cr < half_w) {
    // fill top/bottom borders & center
    _rectangle(x+cr, y, w-cr2, h);
    if (cr < half_h) {
      // fill left border
      _rectangle(x, y+cr, cr, h-cr2);
      // fill right border
      _rectangle(x+w-cr, y+cr, cr, h-cr2);
    }
  } else if (cr < half_h) {
    // fill left/right borders
    _rectangle(x, y+cr, w, h-cr2);
  }
}

void DrawContext::border(const Rect& r, float borderWidth)
{
  if ((_color0 | _color1) == 0) { return; }

  const auto [x,y,w,h] = r;
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
    _dl->quad2(A,B,iA,iB); // top
    _dl->quad2(iC,iD,C,D); // bottom
    _dl->quad2(A,iA,C,iC); // left
    _dl->quad2(iB,B,iD,D); // right
  } else {
    const Vertex2C v_A{A.x, A.y, pointColor(A)};
    const Vertex2C v_B{B.x, B.y, pointColor(B)};
    const Vertex2C v_C{C.x, C.y, pointColor(C)};
    const Vertex2C v_D{D.x, D.y, pointColor(D)};

    const Vertex2C v_iA{iA.x, iA.y, pointColor(iA)};
    const Vertex2C v_iB{iB.x, iB.y, pointColor(iB)};
    const Vertex2C v_iC{iC.x, iC.y, pointColor(iC)};
    const Vertex2C v_iD{iD.x, iD.y, pointColor(iD)};

    quad(v_A, v_B, v_iA, v_iB);
    quad(v_iC, v_iD, v_C, v_D);
    quad(v_A, v_iA, v_C, v_iC);
    quad(v_iB, v_B, v_iD, v_D);
  }
}

void DrawContext::borderShaded(
  const Rect& r, float borderWidth,
  RGBA8 innerColor, RGBA8 outerColor, RGBA8 fillColor)
{
  if ((innerColor | outerColor | fillColor) == 0) { return; }

  float x0 = r.x, y0 = r.y, x1 = r.x+r.w, y1 = r.y+r.h;
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
  const Rect& r, float curveRadius, int curveSegments, float borderWidth)
{
  if (!checkColor()) { return; }

  const auto [x,y,w,h] = r;
  const float half_w = w * .5f;
  const float half_h = h * .5f;
  const float cr = min3(curveRadius, half_w, half_h);

  // corners
  constexpr float a90  = degToRad(90.0f);
  constexpr float a180 = degToRad(180.0f);
  constexpr float a270 = degToRad(270.0f);
  constexpr float a360 = degToRad(360.0f);
  _arc({x+cr,y+cr}, cr, a270, a360, curveSegments, borderWidth);    // top/left
  _arc({x+w-cr,y+cr}, cr, 0, a90, curveSegments, borderWidth);      // top/right
  _arc({x+w-cr,y+h-cr}, cr, a90, a180, curveSegments, borderWidth); // bottom/right
  _arc({x+cr,y+h-cr}, cr, a180, a270, curveSegments, borderWidth);  // bottom/left

  // borders/center
  if (cr < half_w) {
    // top/bottom borders
    const float bw = w - (cr * 2.0f);
    _rectangle(x+cr, y, bw, borderWidth);
    _rectangle(x+cr, y+h-borderWidth, bw, borderWidth);
  }

  if (cr < half_h) {
    // left/right borders
    const float bh = h - (cr * 2.0f);
    _rectangle(x, y+cr, borderWidth, bh);
    _rectangle(x+w-borderWidth, y+cr, borderWidth, bh);
  }
}

void DrawContext::roundedBorderShaded(
  const Rect& r, float curveRadius, int curveSegments, float borderWidth,
  RGBA8 innerColor, RGBA8 outerColor, RGBA8 fillColor)
{
  if ((innerColor | outerColor | fillColor) == 0) { return; }

  const auto [x,y,w,h] = r;
  const float half_w = w * .5f;
  const float half_h = h * .5f;
  const float cr = min3(curveRadius, half_w, half_h);

  // corners
  constexpr float a90  = degToRad(90.0f);
  constexpr float a180 = degToRad(180.0f);
  constexpr float a270 = degToRad(270.0f);
  constexpr float a360 = degToRad(360.0f);
  _arcShaded({x+cr,y+cr}, cr, a270, a360, curveSegments, borderWidth,
             innerColor, outerColor, fillColor); // top/left
  _arcShaded({x+w-cr,y+cr}, cr, 0, a90, curveSegments, borderWidth,
             innerColor, outerColor, fillColor); // top/right
  _arcShaded({x+w-cr,y+h-cr}, cr, a90, a180, curveSegments, borderWidth,
             innerColor, outerColor, fillColor); // bottom/right
  _arcShaded({x+cr,y+h-cr}, cr, a180, a270, curveSegments, borderWidth,
             innerColor, outerColor, fillColor); // bottom/left

  // borders/center
  if (cr < half_w) {
    // top/bottom borders
    const float x0 = x + cr;
    const float x1 = x + w - cr;
    _dl->quad2C({x0, y,               outerColor},
                {x1, y,               outerColor},
                {x0, y+borderWidth,   innerColor},
                {x1, y+borderWidth,   innerColor});
    _dl->quad2C({x0, y+h-borderWidth, innerColor},
                {x1, y+h-borderWidth, innerColor},
                {x0, y+h,             outerColor},
                {x1, y+h,             outerColor});
  }

  if (cr < half_h) {
    // left/right borders
    const float y0 = y + cr;
    const float y1 = y + h - cr;
    _dl->quad2C({x,               y0, outerColor},
                {x+borderWidth,   y0, innerColor},
                {x,               y1, outerColor},
                {x+borderWidth,   y1, innerColor});
    _dl->quad2C({x+w-borderWidth, y0, innerColor},
                {x+w,             y0, outerColor},
                {x+w-borderWidth, y1, innerColor},
                {x+w,             y1, outerColor});
  }

  if (fillColor) {
    if (cr < half_w) {
      // fill top/bottom borders & center
      _dl->quad2C({x+cr,   y+borderWidth,   fillColor},
                  {x+w-cr, y+borderWidth,   fillColor},
                  {x+cr,   y+h-borderWidth, fillColor},
                  {x+w-cr, y+h-borderWidth, fillColor});
      if (cr < half_h) {
        const float y0 = y + cr;
        const float y1 = y + h - cr;

        // fill left border
        _dl->quad2C({x+borderWidth,   y0, fillColor},
                    {x+cr,            y0, fillColor},
                    {x+borderWidth,   y1, fillColor},
                    {x+cr,            y1, fillColor});
        // fill right border
        _dl->quad2C({x+w-cr,          y0, fillColor},
                    {x+w-borderWidth, y0, fillColor},
                    {x+w-cr,          y1, fillColor},
                    {x+w-borderWidth, y1, fillColor});
      }
    } else if (cr < half_h) {
      // fill left/right borders & center
      _dl->quad2C({x,   y+cr,   fillColor},
                  {x+w, y+cr,   fillColor},
                  {x,   y+h-cr, fillColor},
                  {x+w, y+h-cr, fillColor});
    }
  }
}

void DrawContext::shape(const Rect& r, const Style& style)
{
  if (style.fill != Style::no_fill) {
    switch (style.fill) {
      default: // Style::solid
        color(style.fillColor);
        break;
      case Style::hgradient:
        hgradient(r.x, style.fillColor, r.x+r.w, style.fillColor2);
        break;
      case Style::vgradient:
        vgradient(r.y, style.fillColor, r.y+r.h, style.fillColor2);
        break;
    }

    if (isLTE(style.cornerRadius, 0.0f)) {
      rectangle(r);
    } else {
      roundedRectangle(r, style.cornerRadius, style.cornerSegments);
    }
  }

  if (style.edge != Style::no_edge && style.edgeColor != 0) {
    color(style.edgeColor);
    if (isLTE(style.cornerRadius, 0.0f)) {
      switch (style.edge) {
        default: // Style::border_1px
          border(r, 1);
          break;
        case Style::border_2px:
          border(r, 2);
          break;
        case Style::underline_1px:
          rectangle({r.x, r.y+r.h-1, r.w, 1});
          break;
        case Style::underline_2px:
          rectangle({r.x, r.y+r.h-2, r.w, 2});
          break;
        case Style::overline_1px:
          rectangle({r.x, r.y, r.w, 1});
          break;
        case Style::overline_2px:
          rectangle({r.x, r.y, r.w, 2});
          break;
      }
    } else {
      switch (style.edge) {
        default: // Style::border_1px
          roundedBorder(r, style.cornerRadius, style.cornerSegments, 1);
          break;
        case Style::border_2px:
          roundedBorder(r, style.cornerRadius, style.cornerSegments, 2);
          break;
        case Style::underline_1px:
          rectangle(
            {r.x+style.cornerRadius, r.y+r.h-1, r.w-(style.cornerRadius*2), 1});
          break;
        case Style::underline_2px:
          rectangle(
            {r.x+style.cornerRadius, r.y+r.h-2, r.w-(style.cornerRadius*2), 2});
          break;
        case Style::overline_1px:
          rectangle(
            {r.x+style.cornerRadius, r.y, r.w-(style.cornerRadius*2), 1});
          break;
        case Style::overline_2px:
          rectangle(
            {r.x+style.cornerRadius, r.y, r.w-(style.cornerRadius*2), 2});
          break;
      }
    }
  }
}

RGBA8 DrawContext::gradientColor(float g) const
{
  if (g <= _g0) { return _color0; }
  else if (g >= _g1) { return _color1; }

  const float t = (g - _g0) / (_g1 - _g0);
  return packRGBA8((_fullcolor0 * (1.0f-t)) + (_fullcolor1 * t));
}
