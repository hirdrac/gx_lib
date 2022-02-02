//
// demo_draw.cc
// Copyright (C) 2022 Richard Bradley
//

// TODO: next/previous page when all items don't fit in window

#include "gx/Window.hh"
#include "gx/Renderer.hh"
#include "gx/Font.hh"
#include "gx/DrawContext.hh"
#include "gx/Print.hh"


// **** Constants ****
constexpr int DEFAULT_WIDTH = 1280;
constexpr int DEFAULT_HEIGHT = 720;
constexpr int FONT_SIZE = 20;
constexpr int ITEM_WIDTH = 400;
constexpr int ITEM_HEIGHT = 360;

constexpr auto WHITE = gx::packRGBA8(gx::WHITE);
constexpr auto GRAY50 = gx::packRGBA8(gx::GRAY50);
constexpr auto BLACK = gx::packRGBA8(gx::BLACK);
constexpr auto RED = gx::packRGBA8(1.0f, 0, 0, 1.0f);


// **** Draw Functions ****
void draw_circle1(gx::DrawContext& dc, float x, float y)
{
  dc.color(GRAY50);
  dc.circleSector({x + 200, y + 180}, 150, 0, 0, 16);
}

void draw_circle2(gx::DrawContext& dc, float x, float y)
{
  dc.color(GRAY50);
  dc.circleSector({x + 200, y + 180}, 150, 20, 270, 16);
}

void draw_circle3(gx::DrawContext& dc, float x, float y)
{
  dc.circleSector({x + 200, y + 180}, 150, 0, 0, 32, RED, WHITE);
}

void draw_circle4(gx::DrawContext& dc, float x, float y)
{
  dc.circleSector({x + 200, y + 180}, 150, 20, 270, 32, BLACK, WHITE);
}

void draw_circle5(gx::DrawContext& dc, float x, float y)
{
  dc.hgradient(x+50, BLACK, x+350, WHITE);
  dc.circleSector({x + 200, y + 180}, 150, 0, 0, 32);
}

void draw_circle6(gx::DrawContext& dc, float x, float y)
{
  dc.vgradient(y+20, BLACK, y+330, WHITE);
  dc.circleSector({x + 200, y + 180}, 150, 0, 0, 32);
}

void draw_rrect1(gx::DrawContext& dc, float x, float y)
{
  dc.color(GRAY50);
  dc.roundedRectangle(x+20, y+30, 360, 300, 60, 4);
}

void draw_rrect2(gx::DrawContext& dc, float x, float y)
{
  dc.hgradient(x+20, WHITE, x+380, BLACK);
  dc.roundedRectangle(x+20, y+30, 360, 300, 60, 4);
}

void draw_rrect3(gx::DrawContext& dc, float x, float y)
{
  dc.vgradient(y+30, WHITE, y+330, BLACK);
  dc.roundedRectangle(x+20, y+30, 360, 300, 60, 4);
}

void draw_rrect4(gx::DrawContext& dc, float x, float y)
{
  dc.color(WHITE);
  dc.roundedRectangle(x+150, y+30, 100, 300, 60, 4);
}

void draw_rrect5(gx::DrawContext& dc, float x, float y)
{
  dc.color(WHITE);
  dc.roundedRectangle(x+20, y+130, 360, 100, 60, 4);
}

void draw_rrect6(gx::DrawContext& dc, float x, float y)
{
  dc.color(WHITE);
  dc.roundedRectangle(x+150, y+130, 100, 100, 60, 4);
}

void draw_arc1(gx::DrawContext& dc, float x, float y)
{
  dc.color(WHITE);
  dc.arc({x + 200, y + 180}, 150, 0, 0, 32, 1);
}

void draw_arc2(gx::DrawContext& dc, float x, float y)
{
  dc.color(WHITE);
  dc.arc({x + 200, y + 180}, 150, 20, 270, 32, 16);
}

void draw_border1(gx::DrawContext& dc, float x, float y)
{
  dc.color(GRAY50);
  dc.border(x+20, y+30, 360, 300, 4.0f);
}

void draw_border2(gx::DrawContext& dc, float x, float y)
{
  dc.hgradient(x+20, BLACK, x+380, WHITE);
  dc.border(x+20, y+30, 360, 300, 8.0f);
}

void draw_border3(gx::DrawContext& dc, float x, float y)
{
  dc.vgradient(y+30, BLACK, y+330, WHITE);
  dc.border(x+20, y+30, 360, 300, 8.0f);
}

void draw_rborder1(gx::DrawContext& dc, float x, float y)
{
  dc.color(GRAY50);
  dc.roundedBorder(x+20, y+30, 360, 300, 40, 4, 4.0f);
}

void draw_rborder2(gx::DrawContext& dc, float x, float y)
{
  dc.hgradient(x+20, WHITE, x+380, BLACK);
  dc.roundedBorder(x+20, y+30, 360, 300, 40, 4, 8.0f);
}

void draw_rborder3(gx::DrawContext& dc, float x, float y)
{
  dc.vgradient(y+30, WHITE, y+330, BLACK);
  dc.roundedBorder(x+20, y+30, 360, 300, 40, 4, 8.0f);
}


struct { const char* desc; void(*fn)(gx::DrawContext&,float,float); }
  functions[] = {
  {"Solid Circle", draw_circle1},
  {"Partial Circle", draw_circle2},
  {"Gradient Full Circle", draw_circle3},
  {"Gradient Partial Circle", draw_circle4},
  {"HGradient Circle", draw_circle5},
  {"VGradient Circle", draw_circle6},
  {"Rounded Rectangle", draw_rrect1},
  {"HGradient Rounded Rect", draw_rrect2},
  {"VGradient Rounded Rect", draw_rrect3},
  {"Narrow Width Rounded Rect", draw_rrect4},
  {"Narrow Height Rounded Rect", draw_rrect5},
  {"Narrow Width/Height Rounded Rect", draw_rrect6},
  {"Full Arc", draw_arc1},
  {"Partial Arc", draw_arc2},
  {"Border", draw_border1},
  {"HGradient Border", draw_border2},
  {"VGradient Border", draw_border3},
  {"Rounded Border", draw_rborder1},
  {"HGradient Rounded Border", draw_rborder2},
  {"VGradient Rounded Border", draw_rborder3},
};


// **** main ****
int main(int argc, char** argv)
{
  gx::Font fnt{FONT_SIZE};
  if (!fnt.load("data/FreeSans.ttf")) {
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
  fnt.makeAtlas(ren);

  gx::TextFormatting tf{&fnt};

  for (;;) {
    // draw frame
    if (win.resized()) {
      gx::DrawList dl;
      gx::DrawContext dc{dl};

      // draw function & text split to reduce draw calls
      float x = 0, y = 0;
      for (auto& [desc,fn] : functions) {
        fn(dc, x, y);
        x += ITEM_WIDTH;
        if (x > float(win.width() - ITEM_WIDTH)) { x = 0; y += ITEM_HEIGHT; }
      }

      dc.color(WHITE);
      x = y = 0;
      for (auto& [desc,fn] : functions) {
        dc.text(tf, x+(ITEM_WIDTH/2), y+6, gx::ALIGN_TOP_CENTER, desc);
        x += ITEM_WIDTH;
        if (x > float(win.width() - ITEM_WIDTH)) { x = 0; y += ITEM_HEIGHT; }
      }

      ren.clearFrame(win.width(), win.height());
      ren.draw(dl);
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
  }

  return 0;
}
