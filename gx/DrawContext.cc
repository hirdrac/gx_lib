//
// gx/DrawContext.cc
// Copyright (C) 2022 Richard Bradley
//

#include "DrawContext.hh"
#include "Font.hh"
#include "Unicode.hh"
#include "MathUtil.hh"
#include <cassert>
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
    return abs((ba.x * ca.y) - (ba.y * ca.x)) * .5f;
  }

  inline void fixAngles(float& startAngle, float& endAngle) {
    while (endAngle <= startAngle) { endAngle += 360.0f; }
    endAngle = std::min(endAngle, startAngle + 360.0f);
  }
}


void DrawContext::rectangle(float x, float y, float w, float h)
{
  if (checkColor()) { _rectangle(x, y, w, h); }
}

void DrawContext::_rectangle(float x, float y, float w, float h)
{
  if (_colorMode == CM_SOLID) {
    _rect(x, y, w, h);
  } else {
    const Vec2 A{x,y};
    const Vec2 B{x+w,y};
    const Vec2 C{x,y+h};
    const Vec2 D{x+w,y+h};
    add(CMD_quad2C,
        A.x, A.y, pointColor(A),
        B.x, B.y, pointColor(B),
        C.x, C.y, pointColor(C),
        D.x, D.y, pointColor(D));
  }
}

void DrawContext::rectangle(
  float x, float y, float w, float h, Vec2 t0, Vec2 t1)
{
  if (checkColor()) { _rectangle(x, y, w, h, t0, t1); }
}

void DrawContext::_rectangle(
  float x, float y, float w, float h, Vec2 t0, Vec2 t1)
{
  if (_colorMode == CM_SOLID) {
    add(CMD_rectangleT, x, y, t0.x, t0.y, x+w, y+h, t1.x, t1.y);
  } else {
    const Vec2 A{x,y};
    const Vec2 B{x+w,y};
    const Vec2 C{x,y+h};
    const Vec2 D{x+w,y+h};
    add(CMD_quad2TC,
        A.x, A.y, t0.x, t0.y, pointColor(A),
        B.x, B.y, t1.x, t0.y, pointColor(B),
        C.x, C.y, t0.x, t1.y, pointColor(C),
        D.x, D.y, t1.x, t1.y, pointColor(D));
  }
}

void DrawContext::rectangle(
  float x, float y, float w, float h, Vec2 t0, Vec2 t1, const Rect& clip)
{
  if (!checkColor()) { return; }

  float x0 = x;
  float y0 = y;
  float x1 = x + w;
  float y1 = y + h;
  const float cx0 = clip.x;
  const float cy0 = clip.y;
  const float cx1 = clip.x + clip.w;
  const float cy1 = clip.y + clip.h;

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
  const TextFormatting& tf, float x, float y, AlignEnum align, int code)
{
  if (!checkColor()) { return; }

  assert(tf.font != nullptr);
  const Font& f = *tf.font;
  const Glyph* g = f.findGlyph(code);
  if (!g) {
    g = f.findGlyph(tf.unknownCode);
    assert(g != nullptr);
  }

  if (!g->bitmap) { return; }

  Vec2 cursor{x,y};
  const AlignEnum v_align = VAlign(align);
  if (v_align == ALIGN_TOP) {
    cursor += tf.advY * f.ymax();
  } else {
    const float fs = float(f.size()) + tf.lineSpacing;
    if (v_align == ALIGN_BOTTOM) {
      cursor += tf.advY * (f.ymin() - fs);
    } else { // ALIGN_VCENTER
      cursor += tf.advY * ((f.ymax() - fs) * .5f);
    }
  }

  const AlignEnum h_align = HAlign(align);
  if (h_align != ALIGN_LEFT) {
    const float tw = f.glyphWidth(code);
    cursor -= tf.advX * ((h_align == ALIGN_RIGHT) ? tw : (tw * .5f));
  }

  texture(f.tex());
  _glyph(*g, tf, cursor, nullptr);
}

void DrawContext::_text(
  const TextFormatting& tf, float x, float y, AlignEnum align,
  std::string_view text, const Rect* clipPtr)
{
  if (text.empty() || !checkColor()) { return; }

  assert(tf.font != nullptr);
  const Font& f = *tf.font;
  const float fs = float(f.size()) + tf.lineSpacing;
  const AlignEnum h_align = HAlign(align);
  const AlignEnum v_align = VAlign(align);
  Vec2 cursor{x,y};

  if (v_align == ALIGN_TOP) {
    cursor += tf.advY * f.ymax();
  } else {
    int nl = 0;
    for (int ch : text) { nl += (ch == '\n'); }
    if (v_align == ALIGN_BOTTOM) {
      cursor += tf.advY * (f.ymin() - (fs*float(nl)));
    } else { // ALIGN_VCENTER
      cursor += tf.advY * ((f.ymax() - (fs*float(nl))) * .5f);
    }
  }

  texture(f.tex());
  std::size_t lineStart = 0;
  Vec2 startCursor = cursor;

  for (;;) {
    const std::size_t pos = text.find('\n', lineStart);
    const std::string_view line = text.substr(
      lineStart, (pos != std::string_view::npos) ? (pos - lineStart) : pos);

    if (!line.empty()) {
      if (h_align != ALIGN_LEFT) {
        const float tw = f.calcLength(line, tf.glyphSpacing);
        cursor -= tf.advX * ((h_align == ALIGN_RIGHT) ? tw : (tw * .5f));
      }

      for (UTF8Iterator itr{line}; !itr.done(); itr.next()) {
        int ch = itr.get();
        if (ch == '\t') { ch = ' '; }
        const Glyph* g = f.findGlyph(ch);
        if (!g) {
          g = f.findGlyph(tf.unknownCode);
          assert(g != nullptr);
        }

        if (g->bitmap) { _glyph(*g, tf, cursor, clipPtr); }
        cursor += tf.advX * (g->advX + tf.glyphSpacing);
      }
    }

    if (pos == std::string_view::npos) { break; }

    // move to start of next line
    lineStart = pos + 1;
    startCursor += tf.advY * fs;
    cursor = startCursor;
  }
}

void DrawContext::_glyph(
  const Glyph& g, const TextFormatting& tf, Vec2 cursor, const Rect* clipPtr)
{
  const Vec2 gx = tf.glyphX * float(g.width);
  const Vec2 gy = tf.glyphY * float(g.height);

  // quad: A-B
  //       |/|
  //       C-D

  Vec2 A = cursor + (tf.glyphX * g.left) - (tf.glyphY * g.top);
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
    Vec2 newAt = At, newBt = Bt, newCt = Ct, newDt = Dt;

    // use barycentric coordinates to update texture coords
    // P = uA + vB + wC
    // u = triangle_BCP_area / triangle_ABC_area
    // v = triangle_CAP_area / triangle_ABC_area
    // w = triangle_ABP_area / triangle_ABC_area
    //     (if inside triangle, w = 1 - u - v)

    if (A != newA) {
      const float area = triangleArea(A,B,C);
      const float u = triangleArea(B,C,newA) / area;
      const float v = triangleArea(C,A,newA) / area;
      const float w = 1.0f - u - v; // A,B,newA
      newAt = At*u + Bt*v + Ct*w;
    }

    if (B != newB) {
      const float area = triangleArea(B,D,A);
      const float u = triangleArea(D,A,newB) / area;
      const float v = triangleArea(C,B,newB) / area;
      const float w = 1.0f - u - v; // B,D,newB
      newBt = Bt*u + Dt*v + A*w;
    }

    if (C != newC) {
      const float area = triangleArea(C,A,D);
      const float u = triangleArea(A,D,newC) / area;
      const float v = triangleArea(D,C,newC) / area;
      const float w = 1.0f - u - v; // C,A,newC
      newCt = Ct*u + At*v + Dt*w;
    }

    if (D != newD) {
      const float area = triangleArea(D,C,B);
      const float u = triangleArea(C,B,newD) / area;
      const float v = triangleArea(B,D,newD) / area;
      const float w = 1.0f - u - v; // D,C,newD
      newDt = Dt*u + Ct*v + Bt*w;
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
  _circleSector(center, radius, startAngle, endAngle, segments);
}

void DrawContext::_circleSector(
  Vec2 center, float radius, float startAngle, float endAngle, int segments)
{
  const float angle0 = degToRad(startAngle);
  const float angle1 = degToRad(endAngle);
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
      _triangle(v0, v1, v2);
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
  RGBA8 color0, RGBA8 color1)
{
  if ((color0 | color1) == 0) { return; }

  fixAngles(startAngle, endAngle);
  const float angle0 = degToRad(startAngle);
  const float angle1 = degToRad(endAngle);
  const float segmentAngle = (angle1 - angle0) / float(segments);

  const Vertex2C v0{center.x, center.y, color0};

  Vertex2C v1{
    center.x + (radius * std::sin(angle0)),
    center.y - (radius * std::cos(angle0)),
    color1};

  Vertex2C v2;
  v2.c = color1;

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
  _arc(center, radius, startAngle, endAngle, segments, arcWidth);
}

void DrawContext::_arc(
  Vec2 center, float radius, float startAngle, float endAngle,
  int segments, float arcWidth)
{
  const float angle0 = degToRad(startAngle);
  const float angle1 = degToRad(endAngle);
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

void DrawContext::roundedRectangle(
  float x, float y, float w, float h, float curveRadius, int curveSegments)
{
  if (!checkColor()) { return; }

  const float half_w = w * .5f;
  const float half_h = h * .5f;
  const float r = std::min(curveRadius, std::min(half_w, half_h));

  // corners
  _circleSector({x+r,y+r}, r, 270, 360, curveSegments);    // top/left
  _circleSector({x+w-r,y+r}, r, 0, 90, curveSegments);     // top/right
  _circleSector({x+w-r,y+h-r}, r, 90, 180, curveSegments); // bottom/right
  _circleSector({x+r,y+h-r}, r, 180, 270, curveSegments);  // bottom/left

  // borders/center
  if (r == curveRadius) {
    // can fit all borders
    const float rr = r * 2.0f;
    _rectangle(x+r, y, w - rr, r);
    _rectangle(x, y+r, w, h - rr);
    _rectangle(x+r, y+h-r, w - rr, r);
  } else if (r < half_w) {
    // can only fit top/bottom borders
    _rectangle(x+r, y, w - (r*2.0f), h);
  } else if (r < half_h) {
    // can only fit left/right borders
    _rectangle(x, y+r, w, h - (r*2.0f));
  }
}

void DrawContext::border(float x, float y, float w, float h, float borderWidth)
{
  if (!checkColor()) { return; }

  const Vec2 A{x,y};
  const Vec2 B{x+w,y};
  const Vec2 C{x,y+h};
  const Vec2 D{x+w,y+h};

  const Vec2 iA{x+borderWidth,y+borderWidth};
  const Vec2 iB{x+w-borderWidth,y+borderWidth};
  const Vec2 iC{x+borderWidth,y+h-borderWidth};
  const Vec2 iD{x+w-borderWidth,y+h-borderWidth};

  if (_colorMode == CM_SOLID) {
    _quad(A,B,iA,iB); // top
    _quad(iC,iD,C,D); // bottom
    _quad(A,iA,C,iC); // left
    _quad(iB,B,iD,D); // right
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

void DrawContext::roundedBorder(
  float x, float y, float w, float h,
  float curveRadius, int curveSegments, float borderWidth)
{
  if (!checkColor()) { return; }

  const float half_w = w * .5f;
  const float half_h = h * .5f;
  const float r = std::min(curveRadius, std::min(half_w, half_h));

  // corners
  _arc({x+r,y+r}, r, 270, 360, curveSegments, borderWidth);    // top/left
  _arc({x+w-r,y+r}, r, 0, 90, curveSegments, borderWidth);     // top/right
  _arc({x+w-r,y+h-r}, r, 90, 180, curveSegments, borderWidth); // bottom/right
  _arc({x+r,y+h-r}, r, 180, 270, curveSegments, borderWidth);  // bottom/left

  // borders/center
  if (curveRadius < half_w) {
    // top/bottom borders
    const float bw = w - (r * 2.0f);
    _rectangle(x+r, y, bw, borderWidth);
    _rectangle(x+r, y+h-borderWidth, bw, borderWidth);
  }

  if (curveRadius < half_h) {
    // left/right borders
    const float bh = h - (r * 2.0f);
    _rectangle(x, y+r, borderWidth, bh);
    _rectangle(x+w-borderWidth, y+r, borderWidth, bh);
  }
}
