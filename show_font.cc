//
// show_font.cc
// Copyright (C) 2020 Richard Bradley
//
// displays font atlas texture
//

#include "gx/Window.hh"
#include "gx/Font.hh"
#include "gx/Texture.hh"
#include "gx/DrawList.hh"
#include "gx/Print.hh"
#include "gx/StringUtil.hh"


int main(int argc, char** argv)
{
  if (argc < 3) {
    gx::println_err("Usage: ", argv[0], " <TTF font file> <size>");
    return -1;
  }

  std::string fontName = argv[1];
  int fontSize = std::atoi(argv[2]);

  gx::Font fnt;
  if (!fnt.init(fontName, fontSize)) {
    gx::println_err("failed to load font '", fontName, "'");
    return -1;
  }

  gx::Window win;
  win.setTitle(gx::concat(fontName, " - ", fontSize));
  if (!win.open()) {
    gx::println_err("failed to open window");
    return -1;
  }

  fnt.makeAtlas(win);
  const gx::Texture& t = fnt.tex();

  win.setSize(t.width(), t.height(), false);
  win.setBGColor(.3,.1,.1);

  // main loop
  do {
    if (win.resized()) {
      // 'resized' always true once at start
      gx::DrawList dl;
      dl.texture(t);
      dl.rectangle(0, 0, win.width(), win.height(), {0,0}, {1,1});

      win.clearFrame();
      win.draw(dl);
    }

    win.renderFrame();
    win.pollEvents();
  } while (!win.closed() && !win.keyPressCount(gx::KEY_ESCAPE, false));
  return 0;
}
