//
// text_viewer.cc
// Copyright (C) 2021 Richard Bradley
//

// TODO - smooth scrolling
// TODO - line wrap (with indicator)
// TODO - embed default font file, configurable alternate font
// TODO - status bar with filename, current line
// TODO - line number on left side?  (instead of just current line)
// TODO - small text display on side with current view hi-lighted (like VS code)
// TODO - configurable text size
// TODO - goto line GUI

#include "gx/Window.hh"
#include "gx/Renderer.hh"
#include "gx/DrawContext.hh"
#include "gx/Font.hh"
#include "gx/Logger.hh"
#include "gx/Print.hh"

#include <fstream>
#include <string>
#include <vector>
#include <algorithm>


constexpr int DEFAULT_WIDTH = 1280;
constexpr int DEFAULT_HEIGHT = 720;

constexpr int FONT_SIZE = 20;
constexpr int TAB_SIZE = 8;
constexpr int SCROLL_STEP = 3;

// font data from FixedWidthFontData.cc
extern unsigned char FixedWidthFontData[];
extern unsigned long FixedWidthFontDataSize;


int main(int argc, char** argv)
{
  if (argc < 2) {
    gx::println_err("Usage: ", argv[0], " <text file>");
    return -1;
  }

  std::string file = argv[1];
  gx::defaultLogger().disable();

  std::ifstream fs(file);
  if (!fs) {
    gx::println_err("Can't read file '", file, "'");
    return -1;
  }

  std::vector<std::string> text;
  for (;;) {
    std::string line;
    std::getline(fs, line);
    if (fs.eof()) { break; }
    text.push_back(line);
  }
  fs.close();
  if (text.empty()) { text.push_back("* FILE EMPTY *"); }

  gx::Font fnt;
  if (!fnt.loadFromMemory(FixedWidthFontData, FixedWidthFontDataSize, FONT_SIZE)) {
    gx::println_err("failed to load font");
    return -1;
  }

  gx::Window win;
  win.setTitle(file);
  win.setSize(DEFAULT_WIDTH, DEFAULT_HEIGHT, false);
  if (!win.open()) {
    gx::println_err("failed to open window");
    return -1;
  }

  gx::Renderer& ren = win.renderer();
  ren.setBGColor(.2f,.2f,.2f);
  fnt.makeAtlas(ren);

  constexpr int spacing = 0;
  const int lineHeight = fnt.size() + spacing;
  const float tabWidth = fnt.calcWidth(" ") * TAB_SIZE;
  int topLine = 0;

  gx::DrawList dl;
  gx::DrawContext dc(dl);

  // main loop
  bool redraw = true;
  for (;;) {
    const int maxLines = win.height() / lineHeight;
    const int endLine = std::max(int(text.size()) - maxLines, 0);

    // draw frame
    if (win.resized() || redraw) {
      dc.clear();
      dc.color(gx::WHITE);
      dc.tabSize(0.0f, tabWidth);

      float ty = 0.0;
      int pos = topLine;
      while (pos < int(text.size()) && ty < float(win.height())) {
        if (pos >= 0) {
          dc.text(fnt, 0.0f, ty, gx::ALIGN_TOP_LEFT, spacing, text[std::size_t(pos)]);
        }
        ty += float(lineHeight);
        ++pos;
      }

      ren.clearFrame(win.width(), win.height());
      ren.draw(dl);
      redraw = false;
    }
    ren.renderFrame();

    // handle events
    win.pollEvents();
    if (win.closed() || win.keyPressCount(gx::KEY_ESCAPE, true)) { break; }
    if (win.keyPressCount(gx::KEY_F11, false)) {
      if (win.fullScreen()) {
        win.setSize(DEFAULT_WIDTH, DEFAULT_HEIGHT, false);
      } else {
        win.setSize(0, 0, true);
      }
    }

    int lastTop = topLine;
    if (win.keyPressCount(gx::KEY_UP, true)) { --topLine; }
    if (win.keyPressCount(gx::KEY_DOWN, true)) { ++topLine; }
    if (win.keyPressCount(gx::KEY_PAGE_UP, true)) { topLine -= maxLines; }
    if (win.keyPressCount(gx::KEY_PAGE_DOWN, true)) { topLine += maxLines; }
    if (win.keyPressCount(gx::KEY_HOME, false)) { topLine = 0; }
    if (win.keyPressCount(gx::KEY_END, false)) { topLine = endLine; }
    if (win.scrollY() != 0.0f) { topLine -= int(win.scrollY()) * SCROLL_STEP; }

    topLine = std::clamp(topLine, 0, endLine);
    if (lastTop != topLine) { redraw = true; }
  }

  return 0;
}
