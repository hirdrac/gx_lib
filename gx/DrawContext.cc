//
// gx/DrawContext.cc
// Copyright (C) 2021 Richard Bradley
//

#include "DrawContext.hh"
#include "Font.hh"
#include "Unicode.hh"
#include "MathUtil.hh"


namespace {
  [[nodiscard]] constexpr float min4(float a, float b, float c, float d) {
    return std::min(std::min(a,b),std::min(c,d));
  }

  [[nodiscard]] constexpr float max4(float a, float b, float c, float d) {
    return std::max(std::max(a,b),std::max(c,d));
  }

  using gx::Vec2;
  [[nodiscard]] constexpr float triangleArea(Vec2 a, Vec2 b, Vec2 c) {
    // triangle_area = len(cross(B-A,C-A)) / 2
    Vec2 ba = b-a, ca = c-a;
    return gx::abs((ba.x * ca.y) - (ba.y * ca.x)) * .5f;
  }
}


void gx::DrawContext::rectangle(float x, float y, float w, float h)
{
  if (_colorMode == CM_SOLID) {
    _rect(x, y, w, h);
  } else {
    const Vec2 A = {x,y};
    const Vec2 B = {x+w,y};
    const Vec2 C = {x,y+h};
    const Vec2 D = {x+w,y+h};
    add(CMD_quad2C,
        A.x, A.y, pointColor(A),
        B.x, B.y, pointColor(B),
        C.x, C.y, pointColor(C),
        D.x, D.y, pointColor(D));
  }
}

void gx::DrawContext::rectangle(
  float x, float y, float w, float h, Vec2 t0, Vec2 t1)
{
  if (_colorMode == CM_SOLID) {
    add(CMD_rectangleT, x, y, t0.x, t0.y, x+w, y+h, t1.x, t1.y);
  } else {
    const Vec2 A = {x,y};
    const Vec2 B = {x+w,y};
    const Vec2 C = {x,y+h};
    const Vec2 D = {x+w,y+h};
    add(CMD_quad2TC,
        A.x, A.y, t0.x, t0.y, pointColor(A),
        B.x, B.y, t1.x, t0.y, pointColor(B),
        C.x, C.y, t0.x, t1.y, pointColor(C),
        D.x, D.y, t1.x, t1.y, pointColor(D));
  }
}

void gx::DrawContext::rectangle(
  float x, float y, float w, float h, Vec2 t0, Vec2 t1, const Rect& clip)
{
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

void gx::DrawContext::_text(
  const Font& f, const TextFormatting& tf, float x, float y, AlignEnum align,
  std::string_view text, const Rect* clipPtr)
{
  if (text.empty()) { return; }

  const float fs = float(f.size()) + tf.spacing;
  const AlignEnum h_align = HAlign(align);
  const AlignEnum v_align = VAlign(align);
  Vec2 cursor = {x,y};

  if (v_align == ALIGN_TOP) {
    cursor += tf.advY * f.ymax();
  } else {
    int nl = 0;
    for (char ch : text) { nl += int(ch == '\n'); }
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
        const float tw = f.calcWidth(line);
        cursor -= tf.advX * ((h_align == ALIGN_RIGHT) ? tw : (tw * .5f));
      }

      for (UTF8Iterator itr(line); !itr.done(); itr.next()) {
        int ch = itr.get();
        if (ch == '\t') { ch = ' '; }
        const Glyph* g = f.findGlyph(ch);
        if (g) {
          if (g->bitmap) { _glyph(*g, tf, cursor, clipPtr); }
          cursor += tf.advX * g->advX;
        }
      }
    }

    if (pos == std::string_view::npos) { break; }

    // move to start of next line
    lineStart = pos + 1;
    startCursor += tf.advY * fs;
    cursor = startCursor;
  }
}

void gx::DrawContext::_glyph(
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

  Vec2 At = {g.t0.x, g.t0.y};
  Vec2 Bt = {g.t1.x, g.t0.y};
  Vec2 Ct = {g.t0.x, g.t1.y};
  Vec2 Dt = {g.t1.x, g.t1.y};

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

    const Vec2 newA = {std::clamp(A.x,cx0,cx1), std::clamp(A.y,cy0,cy1)};
    const Vec2 newB = {std::clamp(B.x,cx0,cx1), std::clamp(B.y,cy0,cy1)};
    const Vec2 newC = {std::clamp(C.x,cx0,cx1), std::clamp(C.y,cy0,cy1)};
    const Vec2 newD = {std::clamp(D.x,cx0,cx1), std::clamp(D.y,cy0,cy1)};
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

void gx::DrawContext::circleSector(
  Vec2 center, float radius, float startAngle, float endAngle, int segments)
{
  while (endAngle <= startAngle) { endAngle += 360.0f; }
  endAngle = std::min(endAngle, startAngle + 360.0f);

  const float angle0 = degToRad(startAngle);
  const float angle1 = degToRad(endAngle);
  const float segmentAngle = (angle1 - angle0) / float(segments);

  const Vec2 v0 {center.x, center.y};

  Vec2 v1 {
    v1.x = center.x + (radius * std::sin(angle0)),
    v1.y = center.y - (radius * std::cos(angle0))};

  float a = angle0;
  for (int i = 0; i < segments; ++i) {
    if (i == segments-1) { a = angle1; } else { a += segmentAngle; }

    Vec2 v2 {
      center.x + (radius * std::sin(a)),
      center.y - (radius * std::cos(a))};

    triangle(v0, v1, v2);

    // setup for next iteration
    v1.x = v2.x;
    v1.y = v2.y;
  }
}

void gx::DrawContext::circleSector(
  Vec2 center, float radius, float startAngle, float endAngle, int segments,
  uint32_t color0, uint32_t color1)
{
  while (endAngle <= startAngle) { endAngle += 360.0f; }
  endAngle = std::min(endAngle, startAngle + 360.0f);

  const float angle0 = degToRad(startAngle);
  const float angle1 = degToRad(endAngle);
  const float segmentAngle = (angle1 - angle0) / float(segments);

  const Vertex2C v0 {center.x, center.y, color0};

  Vertex2C v1;
  v1.x = center.x + (radius * std::sin(angle0));
  v1.y = center.y - (radius * std::cos(angle0));
  v1.c = color1;

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

void gx::DrawContext::roundedRectangle(
  float x, float y, float w, float h, float curveRadius, int curveSegments)
{
  const float half_w = w * .5f;
  const float half_h = h * .5f;
  const float r = std::min(curveRadius, std::min(half_w, half_h));

  // corners
  circleSector({x+r,y+r}, r, 270, 0, curveSegments);      // top/left
  circleSector({x+w-r,y+r}, r, 0, 90, curveSegments);     // top/right
  circleSector({x+w-r,y+h-r}, r, 90, 180, curveSegments); // bottom/right
  circleSector({x+r,y+h-r}, r, 180, 270, curveSegments);  // bottom/left

  // borders/center
  if (r == curveRadius) {
    // can fit all borders
    const float rr = r * 2.0f;
    _rect(x+r, y, w - rr, r);
    _rect(x, y+r, w, h - rr);
    _rect(x+r, y+h-r, w - rr, r);
  } else if (r < half_w) {
    // can only fit top/bottom borders
    _rect(x+r, y, w - (r*2.0f), h);
  } else if (r < half_h) {
    // can only fit left/right borders
    _rect(x, y+r, w, h - (r*2.0f));
  }
}
