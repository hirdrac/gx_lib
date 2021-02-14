//
// gx/DrawContext.cc
// Copyright (C) 2021 Richard Bradley
//

#include "DrawContext.hh"
#include "Font.hh"
#include "Unicode.hh"


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

  float fs = float(f.size()) + spacing;
  float cursorY;
  switch (VAlign(align)) {
    case ALIGN_TOP:
      cursorY = y + f.ymax();
      break;
    case ALIGN_BOTTOM:
      cursorY = y + f.ymin() - (fs*(lines-1));
      break;
    default: // ALIGN_VCENTER
      cursorY = y + (f.ymax() - (fs*(lines-1))) / 2.0f;
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
          float gx = int(cursorX + g->left);
          float gy = int(cursorY - g->top);

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
