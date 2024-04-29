//
// font_viewer.cc
// Copyright (C) 2024 Richard Bradley
//
// displays font atlas texture
//

#include "gx/Window.hh"
#include "gx/Font.hh"
#include "gx/DrawContext.hh"
#include "gx/Print.hh"
#include "gx/StringUtil.hh"
#include "gx/Unicode.hh"

using gx::println_err;


int main(int argc, char** argv)
{
  if (argc < 3) {
    println_err("Usage: ", argv[0], " <TTF font file> <size>");
    return -1;
  }

  const std::string fontName = argv[1];
  const int fontSize = std::atoi(argv[2]);
  if (fontSize < 1) {
    println_err("invalid size '", argv[2], "'");
    return -1;
  }

  gx::Font fnt{fontSize};
  if (!fnt.load(fontName)) {
    println_err("failed to load font '", fontName, "'");
    return -1;
  }

  gx::Window win;
  win.setTitle(gx::concat(fontName, " - ", fontSize));
  if (!win.open()) {
    println_err("failed to open window");
    return -1;
  }

  fnt.makeAtlas(win);
  const gx::TextureHandle& t = fnt.atlas();

  win.setSize(fnt.atlasWidth(), fnt.atlasHeight(), false);

  gx::DrawList dl;
  gx::DrawContext dc{dl};
  int lastCode = 0;

  // main loop
  const gx::EventState& es = win.eventState();
  for (;;) {
    const auto [width,height] = win.dimensions();
    if (es.resized()) {
      // 'resized' always true once at start
      dc.clearList();
      dc.clearView(.3f,.1f,.1f);
      dc.color(gx::WHITE);
      dc.texture(t);
      dc.rectangle({0, 0, float(width), float(height)}, {0,0}, {1,1});

      win.draw(dl);
    }

    win.renderFrame();

    gx::Window::pollEvents();
    if (es.closed() || es.keyPressCount(gx::KEY_ESCAPE, false)) { break; }

    if (es.mouseIn && (es.events & gx::EVENT_MOUSE_MOVE)) {
      const float tx = es.mousePt.x / float(width);
      const float ty = es.mousePt.y / float(height);
      for (auto& [c,g] : fnt.glyphs()) {
        if (tx >= g.t0.x && tx <= g.t1.x && ty >= g.t0.y && ty <= g.t1.y) {
          if (lastCode != c) {
            println_err("code:",c," '",gx::toUTF8(c),"'");
            lastCode = c;
          }
          break;
        }
      }
    }

    if (es.keyPressCount(gx::KEY_F11, false)) {
      if (win.fullScreen()) {
        win.setSize(fnt.atlasWidth(), fnt.atlasHeight(), false);
      } else {
        win.setSize(0, 0, true);
      }
    }
  }
  return 0;
}
