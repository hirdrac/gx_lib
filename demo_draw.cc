//
// demo_draw.cc
// Copyright (C) 2023 Richard Bradley
//

// TODO: draw 3d cube w/ lighting

#include "gx/Window.hh"
#include "gx/Font.hh"
#include "gx/DrawContext.hh"
#include "gx/Print.hh"
#include "gx/StringUtil.hh"

using gx::println_err;


// **** Constants ****
constexpr int DEFAULT_WIDTH = 1280;
constexpr int DEFAULT_HEIGHT = 800;
constexpr int FONT_SIZE = 20;
constexpr int ITEM_WIDTH = 400;
constexpr int ITEM_HEIGHT = 360;

constexpr auto WHITE = gx::packRGBA8(gx::WHITE);
constexpr auto GRAY50 = gx::packRGBA8(gx::GRAY50);
constexpr auto BLACK = gx::packRGBA8(gx::BLACK);
constexpr auto RED = gx::packRGBA8(1.0f, 0, 0, 1.0f);


// **** Globals ****
gx::Font TheFont{FONT_SIZE};


// **** Draw Functions ****
void draw_circle1(gx::DrawContext& dc, float x, float y)
{
  dc.color(GRAY50);
  dc.circleSector({x + 200, y + 180}, 150, 0, 0, 16);
}

void draw_circle2(gx::DrawContext& dc, float x, float y)
{
  dc.hgradient(x+50, BLACK, x+350, WHITE);
  dc.circleSector({x + 200, y + 180}, 150, 0, 0, 32);
}

void draw_circle3(gx::DrawContext& dc, float x, float y)
{
  dc.vgradient(y+20, BLACK, y+330, WHITE);
  dc.circleSector({x + 200, y + 180}, 150, 0, 0, 32);
}

void draw_circle4(gx::DrawContext& dc, float x, float y)
{
  dc.circleSector({x + 200, y + 180}, 150, 0, 0, 32, RED, WHITE);
}

void draw_circle5(gx::DrawContext& dc, float x, float y)
{
  dc.color(GRAY50);
  dc.circleSector({x + 200, y + 180}, 150, 20, 270, 16);
}

void draw_circle6(gx::DrawContext& dc, float x, float y)
{
  dc.circleSector({x + 200, y + 180}, 150, 20, 270, 32, BLACK, WHITE);
}

void draw_rrect1(gx::DrawContext& dc, float x, float y)
{
  dc.color(GRAY50);
  dc.roundedRectangle({x+20, y+30, 360, 300}, 60, 4);
}

void draw_rrect2(gx::DrawContext& dc, float x, float y)
{
  dc.hgradient(x+20, WHITE, x+380, BLACK);
  dc.roundedRectangle({x+20, y+30, 360, 300}, 60, 4);
}

void draw_rrect3(gx::DrawContext& dc, float x, float y)
{
  dc.vgradient(y+30, WHITE, y+330, BLACK);
  dc.roundedRectangle({x+20, y+30, 360, 300}, 60, 4);
}

void draw_rrect4(gx::DrawContext& dc, float x, float y)
{
  dc.color(WHITE);
  dc.roundedRectangle({x+150, y+30, 100, 300}, 60, 4);
}

void draw_rrect5(gx::DrawContext& dc, float x, float y)
{
  dc.color(WHITE);
  dc.roundedRectangle({x+20, y+130, 360, 100}, 60, 4);
}

void draw_rrect6(gx::DrawContext& dc, float x, float y)
{
  dc.color(WHITE);
  dc.roundedRectangle({x+150, y+130, 100, 100}, 60, 4);
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

void draw_arc3(gx::DrawContext& dc, float x, float y)
{
  dc.arc({x + 200, y + 180}, 150, 20, 270, 32, 32, BLACK, WHITE);
}

void draw_border1(gx::DrawContext& dc, float x, float y)
{
  dc.color(GRAY50);
  dc.border({x+20, y+30, 360, 300}, 4.0f);
}

void draw_border2(gx::DrawContext& dc, float x, float y)
{
  dc.hgradient(x+20, BLACK, x+380, WHITE);
  dc.border({x+20, y+30, 360, 300}, 8.0f);
}

void draw_border3(gx::DrawContext& dc, float x, float y)
{
  dc.vgradient(y+30, BLACK, y+330, WHITE);
  dc.border({x+20, y+30, 360, 300}, 8.0f);
}

void draw_border4(gx::DrawContext& dc, float x, float y)
{
  dc.border({x+20, y+30, 360, 300}, 32.0f, WHITE, gx::packRGBA8(1,1,1,0), 0);
}

void draw_border5(gx::DrawContext& dc, float x, float y)
{
  dc.border({x+20, y+30, 360, 300}, 32.0f, WHITE, gx::packRGBA8(1,1,1,0), RED);
}

void draw_rborder1(gx::DrawContext& dc, float x, float y)
{
  dc.color(GRAY50);
  dc.roundedBorder({x+20, y+30, 360, 300}, 40, 4, 4.0f);
}

void draw_rborder2(gx::DrawContext& dc, float x, float y)
{
  dc.hgradient(x+20, WHITE, x+380, BLACK);
  dc.roundedBorder({x+20, y+30, 360, 300}, 40, 4, 8.0f);
}

void draw_rborder3(gx::DrawContext& dc, float x, float y)
{
  dc.vgradient(y+30, WHITE, y+330, BLACK);
  dc.roundedBorder({x+20, y+30, 360, 300}, 40, 4, 8.0f);
}

void draw_rborder4(gx::DrawContext& dc, float x, float y)
{
  dc.roundedBorder(
    {x+20, y+30, 360, 300}, 48, 4, 32.0f, WHITE, gx::packRGBA8(1,1,1,0), 0);
}

void draw_rborder5(gx::DrawContext& dc, float x, float y)
{
  dc.roundedBorder(
    {x+20, y+30, 360, 300}, 48, 4, 32.0f, WHITE, gx::packRGBA8(1,1,1,0), RED);
}

void draw_lines1(gx::DrawContext& dc, float x, float y)
{
  x += .5f; y += .5f;
  dc.color(WHITE);
  //dc.lineWidth(.5f);
  dc.line(gx::Vec2{x+20,y+30}, gx::Vec2{x+379,y+30});
  dc.line(gx::Vec2{x+20,y+30}, gx::Vec2{x+379,y+180});
  dc.line(gx::Vec2{x+20,y+30}, gx::Vec2{x+379,y+329});
  dc.line(gx::Vec2{x+20,y+30}, gx::Vec2{x+200,y+329});
  dc.line(gx::Vec2{x+20,y+30}, gx::Vec2{x+20,y+329});
}

void draw_lines2(gx::DrawContext& dc, float x, float y)
{
  x += .5f; y += .5f;
  const gx::Vertex2C origin{x+20,y+30,RED};

  dc.line(origin, gx::Vertex2C{x+379,y+30,WHITE});
  dc.line(origin, gx::Vertex2C{x+379,y+180,WHITE});
  dc.line(origin, gx::Vertex2C{x+379,y+329,WHITE});
  dc.line(origin, gx::Vertex2C{x+200,y+329,WHITE});
  dc.line(origin, gx::Vertex2C{x+20,y+329,WHITE});
}

void draw_text1(gx::DrawContext& dc, float x, float y)
{
  gx::TextFormatting tf{&TheFont};
  tf.scale(2.0f);

  for (int i = 0; i < 5; ++i) {
    dc.text(tf, {x+60, y+55}, gx::ALIGN_CENTER_LEFT, "  abc ABC 123");
    tf.rotate(gx::degToRad(22.5f));
  }
}


struct { const char* desc; void(*fn)(gx::DrawContext&,float,float); }
  gfxData[] = {
  {"Circle", draw_circle1},
  {"HGradient Circle", draw_circle2},
  {"VGradient Circle", draw_circle3},
  {"Shaded Circle", draw_circle4},
  {"Partial Circle", draw_circle5},
  {"Shaded Partial Circle", draw_circle6},
  {"Rounded Rectangle", draw_rrect1},
  {"HGradient Rounded Rect", draw_rrect2},
  {"VGradient Rounded Rect", draw_rrect3},
  {"Narrow Width Rounded Rect", draw_rrect4},
  {"Narrow Height Rounded Rect", draw_rrect5},
  {"Narrow Width/Height Rounded Rect", draw_rrect6},
  {"Full Arc", draw_arc1},
  {"Partial Arc", draw_arc2},
  {"Gradient Arc", draw_arc3},
  {"Border", draw_border1},
  {"HGradient Border", draw_border2},
  {"VGradient Border", draw_border3},
  {"Shaded Border", draw_border4},
  {"Shaded Border Filled", draw_border5},
  {"Rounded Border", draw_rborder1},
  {"HGradient Rounded Border", draw_rborder2},
  {"VGradient Rounded Border", draw_rborder3},
  {"Shaded Rounded Border", draw_rborder4},
  {"Shaded Rounded Border Filled", draw_rborder5},
  {"Lines", draw_lines1},
  {"Colored Lines", draw_lines2},
  {"Scaled/Rotated Text", draw_text1},
};


// **** main ****
int main(int argc, char** argv)
{
  if (!TheFont.load("data/FreeSans.ttf")) {
    println_err("failed to load font");
    return -1;
  }

  gx::Window win;
  win.setTitle("draw demo");
  win.setSize(DEFAULT_WIDTH, DEFAULT_HEIGHT, false);
  if (!win.open()) {
    println_err("failed to open window");
    return -1;
  }

  TheFont.makeAtlas(win);

  gx::TextFormatting tf{&TheFont};
  const int gfxCount = std::size(gfxData);
  int page = 0, gfxPerPage = 0, maxPage = 0;
  bool redraw = false;

  gx::DrawList dl;
  gx::DrawContext dc{dl};

  for (;;) {
    // draw frame
    if (win.resized()) {
      page = 0;
      gfxPerPage = std::max((win.width() / ITEM_WIDTH), 1) *
        std::max((win.height() / ITEM_HEIGHT), 1);
      maxPage = (gfxCount - 1) / gfxPerPage;
      redraw = true;
    }

    if (redraw) {
      const int start_gfx = page * gfxPerPage;
      const int end_gfx = std::min(start_gfx + gfxPerPage, gfxCount);
      dc.clearList();
      dc.clearView(.2f,.2f,.5f);

      // draw function & text split to reduce draw calls
      float x = 0, y = 0;
      for (int i = start_gfx; i < end_gfx; ++i) {
        gfxData[i].fn(dc, x, y);
        x += ITEM_WIDTH;
        if (x > float(win.width() - ITEM_WIDTH)) { x = 0; y += ITEM_HEIGHT; }
      }

      dc.color(WHITE);
      x = y = 0;
      for (int i = start_gfx; i < end_gfx; ++i) {
        dc.text(tf, {x+(ITEM_WIDTH/2), y+6}, gx::ALIGN_TOP_CENTER,
                gfxData[i].desc);
        x += ITEM_WIDTH;
        if (x > float(win.width() - ITEM_WIDTH)) { x = 0; y += ITEM_HEIGHT; }
      }

      if (maxPage > 0) {
        dc.text(tf, {float(win.width()-10), float(win.height()-1)},
                gx::ALIGN_BOTTOM_RIGHT,
                gx::concat("Page ", page+1, " of ", maxPage+1));
      }

      win.draw(&dl);
      redraw = false;
    }

    win.renderFrame();

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

    int newPage = page;
    if (win.keyPressCount(gx::KEY_LEFT, true)
        || win.keyPressCount(gx::KEY_UP, true)
        || win.keyPressCount(gx::KEY_PAGE_UP, true)) { --newPage; }
    if (win.keyPressCount(gx::KEY_RIGHT, true)
        || win.keyPressCount(gx::KEY_DOWN, true)
        || win.keyPressCount(gx::KEY_PAGE_DOWN, true)) { ++newPage; }
    if (newPage != page && newPage >= 0 && newPage <= maxPage) {
      page = newPage;
      redraw = true;
    }
  }

  return 0;
}
