//
// text_viewer.cc
// Copyright (C) 2021 Richard Bradley
//

// TODO - smooth scrolling
// TODO - left/right scroll with cursor keys to see long lines
// TODO - optional line wrap (with indicator)
// TODO - status bar with filename, current line
// TODO - line number on left side?  (instead of just current line)
// TODO - small text display on side with current view hi-lighted (like VS code)
// TODO - goto line GUI

#include "gx/Window.hh"
#include "gx/Renderer.hh"
#include "gx/DrawContext.hh"
#include "gx/Font.hh"
#include "gx/Logger.hh"
#include "gx/Print.hh"
#include "gx/CmdLineParser.hh"

#include <fstream>
#include <string>
#include <vector>
#include <algorithm>


constexpr int DEFAULT_WIDTH = 1280;
constexpr int DEFAULT_HEIGHT = 720;
constexpr int DEFAULT_FONT_SIZE = 20;
constexpr int DEFAULT_LINE_SPACING = 0;

constexpr int TAB_SIZE = 8;
constexpr int SCROLL_STEP = 3;

// font data from FixedWidthFontData.cc
extern char FixedWidthFontDataName[];
extern unsigned char FixedWidthFontData[];
extern unsigned long FixedWidthFontDataSize;


int showUsage(char** argv)
{
  gx::println("Usage: ", argv[0], " [options] <text file>");
  gx::println("Options:");
  gx::println("  -f,--font  Font file (defaults to embeded '", FixedWidthFontDataName ,"')");
  gx::println("  -s,--size  Font size (defaults to ", DEFAULT_FONT_SIZE, ")");
  gx::println("  -l,--line  Line spacing (defaults to ", DEFAULT_LINE_SPACING, ")");
  gx::println("  -h,--help  Show usage");
  return 0;
}

int errorUsage(char** argv)
{
  gx::println_err("Try '", argv[0], " --help' for more information.");
  return -1;
}

int main(int argc, char** argv)
{
  if (argc < 2) {
    return showUsage(argv);
  }

  gx::defaultLogger().disable();

  std::string file;
  std::string fontName;
  int fontSize = DEFAULT_FONT_SIZE;
  int lineSpacing = DEFAULT_LINE_SPACING;

  for (gx::CmdLineParser p(argc, argv); p; ++p) {
    if (p.option()) {
      if (p.option('h',"help")) {
        return showUsage(argv);
      } else if (p.option('f',"font", fontName)) {
        if (fontName.empty()) {
          gx::println_err("ERROR: Empty font name");
          return errorUsage(argv);
        }
      } else if (p.option('s',"size", fontSize)) {
        if (fontSize < 1) {
          gx::println_err("ERROR: Bad font size");
          return errorUsage(argv);
        }
      } else if (p.option('l',"line", lineSpacing)) {
        // no lineSpacing value checking currently
      } else {
        gx::println_err("ERROR: Bad option '", p.arg(), "'");
        return errorUsage(argv);
      }
    } else if (file.empty()) {
      p.get(file);
    } else {
      gx::println_err("ERROR: Multiple files not supported");
      return errorUsage(argv);
    }
  }

  if (file.empty()) {
    gx::println_err("ERROR: File name required");
    return errorUsage(argv);
  }

  std::ifstream fs(file);
  if (!fs) {
    gx::println_err("ERROR: Can't read file '", file, "'");
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
  if (fontName.empty()) {
    if (!fnt.loadFromMemory(FixedWidthFontData, FixedWidthFontDataSize, fontSize)) {
      gx::println_err("ERROR: Failed to load embeded font");
      return -1;
    }
  } else if (!fnt.load(fontName, fontSize)) {
    gx::println_err("ERROR: Failed to load font '", fontName, "'");
    return -1;
  }

  gx::Window win;
  win.setTitle(file);
  win.setSize(DEFAULT_WIDTH, DEFAULT_HEIGHT, false);
  if (!win.open()) {
    gx::println_err("ERROR: Failed to open window");
    return -1;
  }

  gx::Renderer& ren = win.renderer();
  ren.setBGColor(.2f,.2f,.2f);
  fnt.makeAtlas(ren);

  const int lineHeight = std::max(fnt.size() + lineSpacing, 1);
  int topLine = 0;

  gx::DrawList dl;
  gx::DrawContext dc(dl);

  gx::TextFormatting tf{};
  tf.tabStart = 0.0f;
  tf.tabWidth = fnt.calcWidth(" ") * TAB_SIZE;
  tf.spacing = 0;

  // main loop
  bool redraw = true;
  for (;;) {
    const int maxLines = win.height() / lineHeight;
    const int endLine = std::max(int(text.size()) - maxLines, 0);

    // draw frame
    if (win.resized() || redraw) {
      dc.clear();
      dc.color(gx::WHITE);

      float ty = 0.0;
      int pos = topLine;
      while (pos < int(text.size()) && ty < float(win.height())) {
        if (pos >= 0) {
          dc.text(fnt, tf, 0.0f, ty, gx::ALIGN_TOP_LEFT, text[std::size_t(pos)]);
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
    gx::Window::pollEvents();
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
