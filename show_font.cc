//
// show_font.cc
// Copyright (C) 2021 Richard Bradley
//
// displays font atlas texture
//

#include "gx/Window.hh"
#include "gx/Renderer.hh"
#include "gx/Font.hh"
#include "gx/Texture.hh"
#include "gx/DrawList.hh"
#include "gx/DrawContext.hh"
#include "gx/Print.hh"
#include "gx/StringUtil.hh"
#include "gx/Unicode.hh"


int main(int argc, char** argv)
{
  if (argc < 3) {
    gx::println_err("Usage: ", argv[0], " <TTF font file> <size>");
    return -1;
  }

  std::string fontName = argv[1];
  int fontSize = std::atoi(argv[2]);

  gx::Font fnt;
  if (!fnt.load(fontName, fontSize)) {
    gx::println_err("failed to load font '", fontName, "'");
    return -1;
  }

  gx::Window win;
  win.setTitle(gx::concat(fontName, " - ", fontSize));
  if (!win.open()) {
    gx::println_err("failed to open window");
    return -1;
  }

  gx::Renderer& ren = win.renderer();
  ren.setBGColor(.3f,.1f,.1f);
  fnt.makeAtlas(ren);
  const gx::Texture& t = fnt.tex();

  win.setSize(t.width(), t.height(), false);

  gx::DrawList dl;
  gx::DrawContext dc(dl);
  int lastCode = 0;

  // main loop
  do {
    if (win.resized()) {
      // 'resized' always true once at start
      dc.clear();
      dc.color(gx::WHITE);
      dc.texture(t);
      dc.rectangle(0, 0, float(win.width()), float(win.height()), {0,0}, {1,1});

      ren.clearFrame(win.width(), win.height());
      ren.draw(dl);
    }

    ren.renderFrame();
    gx::Window::pollEvents();
    if (win.mouseIn() && (win.events() & gx::EVENT_MOUSE_MOVE)) {
      const float tx = win.mouseX() / float(win.width());
      const float ty = win.mouseY() / float(win.height());
      for (auto& [c,g] : fnt.glyphs()) {
        if (tx >= g.t0.x && tx <= g.t1.x && ty >= g.t0.y && ty <= g.t1.y) {
          if (lastCode != c) {
            gx::println_err("code:",c," '",gx::toUTF8(c),"'");
            lastCode = c;
          }
          break;
        }
      }
    }
  } while (!win.closed() && !win.keyPressCount(gx::KEY_ESCAPE, false));
  return 0;
}
