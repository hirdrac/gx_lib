//
// demo_draw.cc
// Copyright (C) 2021 Richard Bradley
//

#include "gx/Window.hh"
#include "gx/Renderer.hh"
#include "gx/Font.hh"
#include "gx/DrawContext.hh"
#include "gx/Print.hh"


// **** Constants ****
constexpr int DEFAULT_WIDTH = 1280;
constexpr int DEFAULT_HEIGHT = 720;
constexpr int ITEM_WIDTH = 400;
constexpr int ITEM_HEIGHT = 360;

constexpr uint32_t WHITE = gx::packRGBA8(gx::WHITE);
constexpr uint32_t BLACK = gx::packRGBA8(gx::BLACK);
constexpr uint32_t RED = gx::packRGBA8(1.0f, 0, 0, 1.0f);


// **** Draw Functions ****
void draw_circle1(gx::DrawContext& dc, float x, float y)
{
  dc.circleSector({x + 200, y + 200}, 150, 0, 0, 32, 0xff000000, 0xffffffff);
}

void draw_circle2(gx::DrawContext& dc, float x, float y)
{
  dc.circleSector({x + 200, y + 200}, 150, 20, 270, 32, 0xff000000, 0xffffffff);
}


struct { const char* desc; void(*fn)(gx::DrawContext&,float,float); }
  functions[] = {
  {"Full Circle", draw_circle1},
  {"Partial Circle", draw_circle2}
};


// **** main ****
int main(int argc, char** argv)
{
  gx::Font fnt;
  if (!fnt.init("data/FreeSans.ttf", 20)) {
    gx::println_err("failed to load font");
    return -1;
  }

  gx::Window win;
  win.setTitle("draw demo");
  win.setSize(DEFAULT_WIDTH, DEFAULT_HEIGHT, false);
  if (!win.open()) {
    gx::println_err("failed to open window");
    return -1;
  }

  gx::Renderer& ren = win.renderer();
  ren.setBGColor(.2f,.2f,.5f);
  fnt.makeAtlas(win);
  bool running = true;
  bool needRedraw = true;

  while (running) {
    // draw frame
    if (win.resized() || needRedraw) {
      gx::DrawList dl;
      gx::DrawContext dc(dl);

      // draw function & text split to reduce draw calls
      float x = 0, y = 0;
      for (auto& [desc,fn] : functions) {
        fn(dc, x, y);
        x += ITEM_WIDTH;
        if (x > float(win.width() - ITEM_WIDTH)) { x = 0; y += ITEM_HEIGHT; }
      }

      dc.color(gx::WHITE);
      x = y = 0;
      for (auto& [desc,fn] : functions) {
        dc.text(fnt, x+2, y+3, gx::ALIGN_TOP_LEFT, 2, desc);
        x += ITEM_WIDTH;
        if (x > float(win.width() - ITEM_WIDTH)) { x = 0; y += ITEM_HEIGHT; }
      }

      ren.clearFrame(win.width(), win.height());
      ren.draw(dl);
      needRedraw = false;
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
  }
}
