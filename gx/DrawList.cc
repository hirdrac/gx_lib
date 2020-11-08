//
// gx/DrawList.cc
// Copyright (C) 2020 Richard Bradley
//

#include "DrawList.hh"
#include "Font.hh"
#include "unicode.hh"


void gx::DrawList::text(const Font& f, float x, float y, AlignEnum align,
			int spacing, std::string_view text)
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
          float xx = cursorX + g->left;
          float yy = cursorY - g->top;

	  // convert x,y to int to make sure text is pixel aligned
	  rectangle(int(xx), int(yy), g->width, g->height, g->t0, g->t1);
        }
        cursorX += g->advX;
      }
    }

    if (pos == std::string_view::npos || pos >= (text.size() - 1)) { break; }
    lineStart = pos + 1;
    cursorY += fs;
  }
}
