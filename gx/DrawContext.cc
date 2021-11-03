//
// gx/DrawContext.cc
// Copyright (C) 2021 Richard Bradley
//

#include "DrawContext.hh"
#include "Font.hh"
#include "Unicode.hh"
#include "MathUtil.hh"


void gx::DrawContext::rectangle(float x, float y, float w, float h)
{
  const float x0 = x;
  const float y0 = y;
  const float x1 = x + w;
  const float y1 = y + h;
  switch (_colorMode) {
    default: {
      add(CMD_rectangle, x0, y0, x1, y1);
      break;
    }
    case CM_HGRADIANT: {
      const uint32_t c0 = gradiantColor(x0);
      const uint32_t c1 = gradiantColor(x1);
      add(CMD_quad2C,
          x0, y0, c0,
          x1, y0, c1,
          x0, y1, c0,
          x1, y1, c1);
      break;
    }
    case CM_VGRADIANT: {
      const uint32_t c0 = gradiantColor(y0);
      const uint32_t c1 = gradiantColor(y1);
      add(CMD_quad2C,
          x0, y0, c0,
          x1, y0, c0,
          x0, y1, c1,
          x1, y1, c1);
      break;
    }
  }
}

void gx::DrawContext::rectangle(
  float x, float y, float w, float h, Vec2 t0, Vec2 t1)
{
  const float x0 = x;
  const float y0 = y;
  const float x1 = x + w;
  const float y1 = y + h;
  switch (_colorMode) {
    default: {
      add(CMD_rectangleT, x0, y0, t0.x, t0.y, x1, y1, t1.x, t1.y);
      break;
    }
    case CM_HGRADIANT: {
      const uint32_t c0 = gradiantColor(x0);
      const uint32_t c1 = gradiantColor(x1);
      add(CMD_quad2TC,
          x0, y0, t0.x, t0.y, c0,
          x1, y0, t1.x, t0.y, c1,
          x0, y1, t0.x, t1.y, c0,
          x1, y1, t1.x, t1.y, c1);
      break;
    }
    case CM_VGRADIANT: {
      const uint32_t c0 = gradiantColor(y0);
      const uint32_t c1 = gradiantColor(y1);
      add(CMD_quad2TC,
          x0, y0, t0.x, t0.y, c0,
          x1, y0, t1.x, t0.y, c0,
          x0, y1, t0.x, t1.y, c1,
          x1, y1, t1.x, t1.y, c1);
      break;
    }
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
    tx0 = tx1 - ((tx1 - tx0) * ((x1 - cx0) / (x1 - x0)));
    x0 = cx0;
  }
  if (x1 > cx1) { // right edge clipped
    tx1 = tx0 + ((tx1 - tx0) * ((cx1 - x0) / (x1 - x0)));
    x1 = cx1;
  }

  float ty0 = t0.y;
  float ty1 = t1.y;
  if (y0 < cy0) { // top clipped
    ty0 = ty1 - ((ty1 - ty0) * ((y1 - cy0) / (y1 - y0)));
    y0 = cy0;
  }
  if (y1 > cy1) { // bottom clipped
    ty1 = ty0 + ((ty1 - ty0) * ((cy1 - y0) / (y1 - y0)));
    y1 = cy1;
  }

  switch (_colorMode) {
    default: {
      add(CMD_rectangleT, x0, y0, tx0, ty0, x1, y1, tx1, ty1);
      break;
    }
    case CM_HGRADIANT: {
      const uint32_t c0 = gradiantColor(x0);
      const uint32_t c1 = gradiantColor(x1);
      add(CMD_quad2TC,
          x0, y0, tx0, ty0, c0,
          x1, y0, tx1, ty0, c1,
          x0, y1, tx0, ty1, c0,
          x1, y1, tx1, ty1, c1);
      break;
    }
    case CM_VGRADIANT: {
      const uint32_t c0 = gradiantColor(y0);
      const uint32_t c1 = gradiantColor(y1);
      add(CMD_quad2TC,
          x0, y0, tx0, ty0, c0,
          x1, y0, tx1, ty0, c0,
          x0, y1, tx0, ty1, c1,
          x1, y1, tx1, ty1, c1);
      break;
    }
  }
}

void gx::DrawContext::_text(
  const Font& f, const TextFormatting& tf, float x, float y, AlignEnum align,
  std::string_view text, const Rect* clipPtr)
{
  if (text.empty()) { return; }

  const float fs = float(f.size() + tf.spacing);
  const AlignEnum h_align = HAlign(align);
  const AlignEnum v_align = VAlign(align);

  float cursorY = y;
  if (v_align == ALIGN_TOP) {
    cursorY += f.ymax();
  } else {
    int nl = 0;
    for (char ch : text) { nl += int(ch == '\n'); }
    if (v_align == ALIGN_BOTTOM) {
      cursorY += f.ymin() - (fs*float(nl));
    } else { // ALIGN_VCENTER
      cursorY += (f.ymax() - (fs*float(nl))) * .5f;
    }
  }

  texture(f.tex());
  std::size_t lineStart = 0;
  float cursorX = x;

  for (;;) {
    const std::size_t pos = (h_align == ALIGN_LEFT)
      ? text.find_first_of("\t\n", lineStart) : text.find('\n', lineStart);
    const int endChar = (pos != std::string_view::npos) ? text[pos] : '\0';

    const std::string_view line = text.substr(
      lineStart, (pos != std::string_view::npos) ? (pos - lineStart) : pos);

    if (!line.empty()) {
      if (h_align != ALIGN_LEFT) {
        const float tw = f.calcWidth(line);
        cursorX -= (h_align == ALIGN_RIGHT) ? tw : (tw * .5f);
      }

      for (UTF8Iterator itr(line); !itr.done(); itr.next()) {
        int ch = itr.get();
        if (ch == '\t') { ch = ' '; }
        const Glyph* g = f.findGlyph(ch);
        if (!g) { continue; }

        if (g->bitmap) {
          // convert x,y to int to make sure text is pixel aligned
          const float gx = float(int(cursorX + g->left));
          const float gy = float(int(cursorY - g->top));
          if (clipPtr) {
            rectangle(gx, gy, g->width, g->height, g->t0, g->t1, *clipPtr);
          } else {
            rectangle(gx, gy, g->width, g->height, g->t0, g->t1);
          }
        }
        cursorX += g->advX;
      }
    }

    if (!endChar) { break; }

    lineStart = pos + 1;
    if (endChar == '\t') {
      // adjust cursorX for tab (only for left alignment)
      const float t = std::floor((cursorX - tf.tabStart) / tf.tabWidth) + 1.0f;
      cursorX = tf.tabStart + (t * tf.tabWidth);
    } else if (endChar == '\n') {
      // move to start of next line
      cursorX = x; cursorY += fs;
    }
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
