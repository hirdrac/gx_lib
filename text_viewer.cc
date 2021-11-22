//
// text_viewer.cc
// Copyright (C) 2021 Richard Bradley
//
// gx_lib example program for text rendering & basic event handling
//

// TODO: smooth scrolling
// TODO: left/right scroll with cursor keys to see long lines
// TODO: optional line wrap (with indicator)
// TODO: status bar with filename, current line
// TODO: line number on left side?  (instead of just current line)
// TODO: small text display on side with current view hi-lighted (like VS code)
// TODO: goto line GUI (control-g)
// TODO: option to show gfx for space/tab/newline characters
// TODO: allow drag (button2/3 down, mouse move) to scroll text
// TODO: text selection and copy (button1 w/ mouse, control-C to copy)
// TODO: find GUI (control-f)

#include "gx/Window.hh"
#include "gx/Renderer.hh"
#include "gx/DrawContext.hh"
#include "gx/Font.hh"
#include "gx/Logger.hh"
#include "gx/Print.hh"
#include "gx/CmdLineParser.hh"

#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>


constexpr int DEFAULT_WIDTH = 1280;
constexpr int DEFAULT_HEIGHT = 720;
constexpr int DEFAULT_FONT_SIZE = 20;
constexpr int DEFAULT_LINE_SPACING = 0;
constexpr int DEFAULT_TAB_SIZE = 8;

constexpr int SCROLL_STEP = 3;

// font data from FixedWidthFontData.cc
extern char FixedWidthFontDataName[];
extern unsigned char FixedWidthFontData[];
extern unsigned long FixedWidthFontDataSize;


int showUsage(char** argv)
{
  gx::println("Usage: ", argv[0], " [options] <text file>");
  gx::println("Options:");
  gx::println("  -f,--font=[]  Font file (defaults to embedded ", FixedWidthFontDataName ,")");
  gx::println("  -s,--size=[]  Font size (default ", DEFAULT_FONT_SIZE, ")");
  gx::println("  -l,--line=[]  Line spacing (default ", DEFAULT_LINE_SPACING, ")");
  gx::println("  -t,--tab=[]   Tab size (default 8)");
  gx::println("  -h,--help     Show usage");
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
  int tabSize = DEFAULT_TAB_SIZE;

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
      } else if (p.option('t',"tab", tabSize)) {
        // no tabSize value checking currently
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
      gx::println_err("ERROR: Failed to load embedded font");
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

  const float tabWidth = fnt.calcWidth(" ") * float(tabSize);
  const int lineHeight = std::max(fnt.size() + lineSpacing, 1);
  int topLine = 0;

  gx::DrawList dl;
  gx::DrawContext dc{dl};
  gx::TextFormatting tf{&fnt};

  // main loop
  bool redraw = true;
  for (;;) {
    const int maxLines = win.height() / lineHeight;
    const int endLine = std::max(int(text.size()) - maxLines, 0);

    // draw frame
    if (win.resized() || redraw) {
      dc.clear();
      dc.color(gx::WHITE);

      float ty = 0;
      int lineNo = topLine;
      while (lineNo < int(text.size()) && ty < float(win.height())) {
        if (lineNo >= 0) {
          const std::string_view line = text[std::size_t(lineNo)];
          float tx = 0;
          std::size_t i = 0;
          while (i < line.size()) {
            const std::size_t tabPos = line.find('\t',i);
            const std::size_t count =
              (tabPos == std::string_view::npos) ? tabPos : tabPos - i;
            const std::string_view segment = line.substr(i,count);
            dc.text(tf, tx, ty, gx::ALIGN_TOP_LEFT, segment);
            i += segment.size() + 1;
            if (tabPos != std::string_view::npos) {
              const float tx2 = tx + fnt.calcWidth(segment);
              tx = (std::floor(tx2 / tabWidth) + 1.0f) * tabWidth;
            }
          }
        }
        ty += float(lineHeight);
        ++lineNo;
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
