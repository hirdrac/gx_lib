//
// text_viewer.cc
// Copyright (C) 2025 Richard Bradley
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
// TODO: change TextBuffer to file buffer & string_view for each line

#include "gx/Window.hh"
#include "gx/EventState.hh"
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
#include <memory_resource>

using gx::println;
using gx::println_err;


constexpr int DEFAULT_WIDTH = 1280;
constexpr int DEFAULT_HEIGHT = 720;
constexpr int DEFAULT_FONT_SIZE = 20;
constexpr int DEFAULT_LINE_SPACING = 0;
constexpr int DEFAULT_GLYPH_SPACING = 0;
constexpr int DEFAULT_TAB_SIZE = 8;

constexpr int SCROLL_STEP = 3;

// font data from FixedWidthFontData.cc
extern char FixedWidthFontDataName[];
extern unsigned char FixedWidthFontData[];
extern unsigned long FixedWidthFontDataSize;


class TextBuffer
{
 public:
  using size_type = std::size_t;
  using string_type = std::pmr::string;
  using char_type = string_type::value_type;
  using view_type = std::basic_string_view<char_type>;
  using vector_type = std::pmr::vector<string_type>;

  [[nodiscard]] bool empty() const { return _text.empty(); }
  [[nodiscard]] size_type lines() const { return _text.size(); }
  [[nodiscard]] view_type operator[](size_type lineNo) const {
    if (lineNo > _text.size()) { return {}; }
    return _text[lineNo];
  }

  void clear() { _text.clear(); }

  bool load(const std::string& file) {
    std::ifstream fs{file};
    if (!fs) { return false; }

    for (;;) {
      string_type line;
      std::getline(fs, line);
      if (fs.eof()) { break; }
      _text.push_back(line);
    }

    return true;
  }

  void addLine(view_type line) { _text.emplace_back(line); }

 private:
  vector_type _text;
};


int showUsage(const char* const* argv)
{
  println("Usage: ", argv[0], " [options] <text file>");
  println("Options:");
  println("  -f,--font=[]          Font file");
  println("                        (defaults to embedded ", FixedWidthFontDataName ,")");
  println("  -s,--size=[]          Font size (default ", DEFAULT_FONT_SIZE, ")");
  println("  -l,--linespacing=[]   Line spacing (default ", DEFAULT_LINE_SPACING, ")");
  println("  -g,--glyphspacing=[]  Glyph spacing (default ", DEFAULT_GLYPH_SPACING, ")");
  println("  -t,--tab=[]           Tab size (default ", DEFAULT_TAB_SIZE, ")");
  println("  -h,--help             Show usage");
  return 0;
}

int errorUsage(const char* const* argv)
{
  println_err("Try '", argv[0], " --help' for more information.");
  return -1;
}

struct ArgParser
{
  std::string file;
  std::string fontName;
  int fontSize = DEFAULT_FONT_SIZE;
  int lineSpacing = DEFAULT_LINE_SPACING;
  int glyphSpacing = DEFAULT_GLYPH_SPACING;
  int tabSize = DEFAULT_TAB_SIZE;

  int parse(int argc, const char* const* argv)
  {
    if (argc < 2) {
      return showUsage(argv);
    }

    for (gx::CmdLineParser p{argc, argv}; p; ++p) {
      if (p.option()) {
        if (p.option('h',"help")) {
          return showUsage(argv);
        } else if (p.option('f',"font", fontName)) {
          if (fontName.empty()) {
            println_err("ERROR: Empty font name");
            return errorUsage(argv);
          }
        } else if (p.option('s',"size", fontSize)) {
          if (fontSize < 1) {
            println_err("ERROR: Bad font size");
            return errorUsage(argv);
          }
        } else if (p.option('l',"linespacing", lineSpacing)) {
          // no lineSpacing value check currently
      } else if (p.option('g',"glyphspacing", glyphSpacing)) {
          // no glyphSpacing value check currently
        } else if (p.option('t',"tab", tabSize)) {
          // no tabSize value check currently
        } else {
          println_err("ERROR: Bad option '", p.arg(), "'");
          return errorUsage(argv);
        }
      } else if (file.empty()) {
        p.get(file);
      } else {
        println_err("ERROR: Multiple files not supported");
        return errorUsage(argv);
      }
    }

    if (file.empty()) {
      println_err("ERROR: File name required");
      return errorUsage(argv);
    }

    return false;
  }
};


int main(int argc, char** argv)
{
  ArgParser args;
  const int error = args.parse(argc, argv);
  if (error) { return error; }

  TextBuffer buffer;
  if (!buffer.load(args.file)) {
    println_err("ERROR: Can't read file '", args.file, "'");
    return -1;
  }

  if (buffer.empty()) { buffer.addLine("* FILE EMPTY *"); }

  gx::defaultLogger().disable();
  gx::Font fnt{args.fontSize};
  if (args.fontName.empty()) {
    if (!fnt.loadFromMemory(FixedWidthFontData, FixedWidthFontDataSize)) {
      println_err("ERROR: Failed to load embedded font");
      return -1;
    }
  } else if (!fnt.load(args.fontName)) {
    println_err("ERROR: Failed to load font '", args.fontName, "'");
    return -1;
  }

  gx::Window win;
  win.setTitle(args.file);
  win.setSize(DEFAULT_WIDTH, DEFAULT_HEIGHT, false);
  if (!win.open()) {
    println_err("ERROR: Failed to open window");
    return -1;
  }

  fnt.makeAtlas(win);

  const int lineHeight = std::max(fnt.size() + args.lineSpacing, 1);
  int topLine = 0;

  gx::DrawList dl;
  gx::DrawContext dc{dl};
  gx::TextFormat tf{&fnt};
  tf.glyphSpacing = float(args.glyphSpacing);
  tf.tabWidth = fnt.glyphWidth(' ') * float(args.tabSize);
  tf.startTag = 0; // disable color tag parsing

  // main loop
  bool redraw = true, running = true;
  while (running) {
    // handle events
    gx::Window::pollEvents();
    const gx::EventState& es = win.eventState();

    if (es.events & gx::EVENT_SIZE) { redraw = true; }
    if (es.events & gx::EVENT_CLOSE) { running = false; }

    const auto [width,height] = win.dimensions();
    const int maxLines = height / lineHeight;
    const int endLine = std::max(int(buffer.lines()) - maxLines, 0);
    const int lastTop = topLine;

    if (es.events & gx::EVENT_KEY) {
      for (const auto& in : es.keyStates) {
        if (!in.pressCount && !in.repeatCount) { continue; }

        switch (in.value) {
          case gx::KEY_ESCAPE:    running = false; break;
          case gx::KEY_UP:        --topLine; break;
          case gx::KEY_DOWN:      ++topLine; break;
          case gx::KEY_PAGE_UP:   topLine -= maxLines; break;
          case gx::KEY_PAGE_DOWN: topLine += maxLines; break;
          case gx::KEY_HOME:      topLine = 0; break;
          case gx::KEY_END:       topLine = endLine; break;

          case gx::KEY_F11:
            if (in.pressCount) {
              if (win.fullScreen()) {
                win.setSize(DEFAULT_WIDTH, DEFAULT_HEIGHT, false);
              } else {
                win.setSize(0, 0, true);
              }
            }
            break;
        }
      }
    }

    if (es.mouseScroll()) {
      topLine -= int(es.scrollPt.y) * SCROLL_STEP;
    }

    topLine = std::clamp(topLine, 0, endLine);
    redraw |= (lastTop != topLine);

    // draw frame
    if (redraw) {
      dc.clearList();
      dc.clearView(.2f,.2f,.2f);

      const float win_w = float(width);
      const float win_h = float(height);

      float ty = 0;
      int lineNo = topLine;
      while (lineNo < int(buffer.lines()) && ty < win_h) {
        if (lineNo >= 0) {
          const std::string_view line = buffer[std::size_t(lineNo)];
          dc.color(gx::WHITE);
          dc.text(tf, {0, ty}, gx::ALIGN_TOP_LEFT, line);
          if (tf.calcLength(line) > win_w) {
            dc.color(1.0f,0,0);
            dc.text(tf, {win_w+1, ty}, gx::ALIGN_TOP_RIGHT, "*");
          }
        }

        ty += float(lineHeight);
        ++lineNo;
      }

      win.draw(dl);
      redraw = false;
    }

    win.renderFrame();
  }

  return 0;
}
