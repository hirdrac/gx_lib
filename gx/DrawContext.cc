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
  if (_colorMode == CM_HGRADIANT) {
    uint32_t c0 = gradiantColor(x0);
    uint32_t c1 = gradiantColor(x1);
    add(CMD_quad2C,
        x0, y0, c0,
        x1, y0, c1,
        x0, y1, c0,
        x1, y1, c1);
  } else if (_colorMode == CM_VGRADIANT) {
    uint32_t c0 = gradiantColor(y0);
    uint32_t c1 = gradiantColor(y1);
    add(CMD_quad2C,
        x0, y0, c0,
        x1, y0, c0,
        x0, y1, c1,
        x1, y1, c1);
  } else {
    add(CMD_rectangle, x0, y0, x1, y1);
  }
}

void gx::DrawContext::rectangle(
  float x, float y, float w, float h, Vec2 t0, Vec2 t1)
{
  const float x0 = x;
  const float y0 = y;
  const float x1 = x + w;
  const float y1 = y + h;
  if (_colorMode == CM_HGRADIANT) {
    uint32_t c0 = gradiantColor(x0);
    uint32_t c1 = gradiantColor(x1);
    add(CMD_quad2TC,
        x0, y0, t0.x, t0.y, c0,
        x1, y0, t1.x, t0.y, c1,
        x0, y1, t0.x, t1.y, c0,
        x1, y1, t1.x, t1.y, c1);
  } else if (_colorMode == CM_VGRADIANT) {
    uint32_t c0 = gradiantColor(y0);
    uint32_t c1 = gradiantColor(y1);
    add(CMD_quad2TC,
        x0, y0, t0.x, t0.y, c0,
        x1, y0, t1.x, t0.y, c0,
        x0, y1, t0.x, t1.y, c1,
        x1, y1, t1.x, t1.y, c1);
  } else {
    add(CMD_rectangleT, x0, y0, t0.x, t0.y, x1, y1, t1.x, t1.y);
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
  float ty0 = t0.y;
  float tx1 = t1.x;
  float ty1 = t1.y;

  if (x0 < cx0) { // left edge clipped
    tx0 = tx1 - ((tx1 - tx0) * ((x1 - cx0) / (x1 - x0)));
    x0 = cx0;
  }
  if (y0 < cy0) { // top clipped
    ty0 = ty1 - ((ty1 - ty0) * ((y1 - cy0) / (y1 - y0)));
    y0 = cy0;
  }
  if (x1 > cx1) { // right edge clipped
    tx1 = tx0 + ((tx1 - tx0) * ((cx1 - x0) / (x1 - x0)));
    x1 = cx1;
  }
  if (y1 > cy1) { // bottom clipped
    ty1 = ty0 + ((ty1 - ty0) * ((cy1 - y0) / (y1 - y0)));
    y1 = cy1;
  }

  if (_colorMode == CM_HGRADIANT) {
    uint32_t c0 = gradiantColor(x0);
    uint32_t c1 = gradiantColor(x1);
    add(CMD_quad2TC,
        x0, y0, tx0, ty0, c0,
        x1, y0, tx1, ty0, c1,
        x0, y1, tx0, ty1, c0,
        x1, y1, tx1, ty1, c1);
  } else if (_colorMode == CM_VGRADIANT) {
    uint32_t c0 = gradiantColor(y0);
    uint32_t c1 = gradiantColor(y1);
    add(CMD_quad2TC,
        x0, y0, tx0, ty0, c0,
        x1, y0, tx1, ty0, c0,
        x0, y1, tx0, ty1, c1,
        x1, y1, tx1, ty1, c1);
  } else {
    add(CMD_rectangleT, x0, y0, tx0, ty0, x1, y1, tx1, ty1);
  }
}

void gx::DrawContext::_text(
  const Font& f, float x, float y, AlignEnum align, int spacing,
  std::string_view text, const Rect* clipPtr)
{
  if (text.empty()) { return; }

  int lines = 1;
  for (char ch : text) { if (ch == '\n') { ++lines; } }

  float fs = float(f.size() + spacing);
  float cursorY;
  switch (VAlign(align)) {
    case ALIGN_TOP:
      cursorY = y + f.ymax();
      break;
    case ALIGN_BOTTOM:
      cursorY = y + f.ymin() - (fs*float(lines-1));
      break;
    default: // ALIGN_VCENTER
      cursorY = y + (f.ymax() - (fs*float(lines-1))) / 2.0f;
      break;
  }

  texture(f.tex());
  AlignEnum h_align = HAlign(align);
  std::size_t lineStart = 0;
  for (;;) {
    std::size_t pos = text.find('\n', lineStart);
    std::string_view line = text.substr(
      lineStart, (pos != std::string_view::npos) ? pos - lineStart : pos);

    if (!line.empty()) {
      float cursorX = x;
      if (h_align != ALIGN_LEFT) {
        float tw = f.calcWidth(line);
        if (h_align == ALIGN_RIGHT) {
          cursorX -= tw;
        } else { // ALIGN_HCENTER
          cursorX -= tw / 2.0f;
        }
      }

      for (UTF8Iterator itr(line); !itr.done(); itr.next()) {
        const Glyph* g = f.findGlyph(itr.get());
        if (!g) { continue; }

        if (g->width > 0 && g->height > 0) {
          // convert x,y to int to make sure text is pixel aligned
          float gx = float(int(cursorX + g->left));
          float gy = float(int(cursorY - g->top));

          if (clipPtr) {
            rectangle(gx, gy, g->width, g->height, g->t0, g->t1, *clipPtr);
          } else {
            rectangle(gx, gy, g->width, g->height, g->t0, g->t1);
          }
        }
        cursorX += g->advX;
      }
    }

    if (pos == std::string_view::npos || pos >= (text.size() - 1)) { break; }
    lineStart = pos + 1;
    cursorY += fs;
  }
}

void gx::DrawContext::circleSector(
  Vec2 center, float radius, float startAngle, float endAngle, int segments)
{
  while (endAngle <= startAngle) { endAngle += 360.0f; }
  endAngle = std::min(endAngle, startAngle + 360.0f);

  const float angle0 = DegToRad(startAngle);
  const float angle1 = DegToRad(endAngle);
  const float segmentAngle = (angle1 - angle0) / float(segments);

  Vec2 v0 {center.x, center.y};
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

  const float angle0 = DegToRad(startAngle);
  const float angle1 = DegToRad(endAngle);
  const float segmentAngle = (angle1 - angle0) / float(segments);

  Vertex2C v0 {center.x, center.y, color0};

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
  float half_w = w / 2.0f;
  float half_h = h / 2.0f;
  float r = std::min(curveRadius, std::min(half_w, half_h));

  // corners
  circleSector({x+r,y+r}, r, 270, 0, curveSegments);      // top/left
  circleSector({x+w-r,y+r}, r, 0, 90, curveSegments);     // top/right
  circleSector({x+w-r,y+h-r}, r, 90, 180, curveSegments); // bottom/right
  circleSector({x+r,y+h-r}, r, 180, 270, curveSegments);  // bottom/left

  // borders/center
  if (r < half_w && r < half_h) {
    // can fit all borders
    rectangle(x+r, y, w - (r*2.0f), r);
    rectangle(x, y+r, w, h - (r*2.0f));
    rectangle(x+r, y+h-r, w - (r*2.0f), r);
  } else if (r < half_w) {
    // can only fit top/bottom borders
    rectangle(x+r, y, w - (r*2.0f), h);
  } else if (r < half_h) {
    // can only fit left/right borders
    rectangle(x, y+r, w, h - (r*2.0f));
  }
}
