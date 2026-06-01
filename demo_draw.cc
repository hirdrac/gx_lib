//
// demo_draw.cc
// Copyright (C) 2026 Richard Bradley
//

// TODO: draw 3d cube w/ lighting
// TODO: test for lineStart/lineTo

#include "gx/Window.hh"
#include "gx/EventState.hh"
#include "gx/Font.hh"
#include "gx/TextFormat.hh"
#include "gx/DrawContext.hh"
#include "gx/Print.hh"
#include "gx/StringUtil.hh"

using gx::println_err;
using gx::Vec2;


// **** Constants ****
constexpr int DEFAULT_WIDTH = 1280;
constexpr int DEFAULT_HEIGHT = 800;
constexpr int FONT_SIZE = 20;
constexpr int ITEM_WIDTH = 400;
constexpr int ITEM_HEIGHT = 360;

constexpr auto WHITE = gx::packRGBA8(gx::WHITE);
constexpr auto GRAY50 = gx::packRGBA8(gx::GRAY50);
constexpr auto BLACK = gx::packRGBA8(gx::BLACK);
constexpr auto RED = gx::packRGBA8(gx::RED);


// **** Globals ****
gx::Font TheFont{FONT_SIZE};


// **** Draw Functions ****
void draw_circle1(gx::DrawContext2D& dc, const gx::Rect& r)
{
  dc.color(GRAY50);
  dc.circle(r.centerPt(), 150, 16);
}

void draw_circle2(gx::DrawContext2D& dc, const gx::Rect& r)
{
  dc.hgradient(r.x+50, BLACK, r.x+350, WHITE);
  dc.circle(r.centerPt(), 150, 32);
}

void draw_circle3(gx::DrawContext2D& dc, const gx::Rect& r)
{
  dc.vgradient(r.y+20, BLACK, r.y+330, WHITE);
  dc.circle(r.centerPt(), 150, 32);
}

void draw_circle4(gx::DrawContext2D& dc, const gx::Rect& r)
{
  dc.circleShaded(r.centerPt(), 150, 32, RED, WHITE);
}

void draw_circle5(gx::DrawContext2D& dc, const gx::Rect& r)
{
  dc.color(GRAY50);
  dc.circleSector(r.centerPt(), 150, 20, 270, 16);
}

void draw_circle6(gx::DrawContext2D& dc, const gx::Rect& r)
{
  dc.circleSectorShaded(r.centerPt(), 150, 20, 270, 32, BLACK, WHITE);
}

void draw_rrect1(gx::DrawContext2D& dc, const gx::Rect& r)
{
  dc.color(GRAY50);
  dc.roundedRectangle({r.x+20, r.y+30, 360, 300}, 60, 4);
}

void draw_rrect2(gx::DrawContext2D& dc, const gx::Rect& r)
{
  dc.hgradient(r.x+20, WHITE, r.x+380, BLACK);
  dc.roundedRectangle({r.x+20, r.y+30, 360, 300}, 60, 4);
}

void draw_rrect3(gx::DrawContext2D& dc, const gx::Rect& r)
{
  dc.vgradient(r.y+30, WHITE, r.y+330, BLACK);
  dc.roundedRectangle({r.x+20, r.y+30, 360, 300}, 60, 4);
}

void draw_rrect4(gx::DrawContext2D& dc, const gx::Rect& r)
{
  dc.color(WHITE);
  dc.roundedRectangle({r.x+150, r.y+30, 100, 300}, 60, 4);
}

void draw_rrect5(gx::DrawContext2D& dc, const gx::Rect& r)
{
  dc.color(WHITE);
  dc.roundedRectangle({r.x+20, r.y+130, 360, 100}, 60, 4);
}

void draw_rrect6(gx::DrawContext2D& dc, const gx::Rect& r)
{
  dc.color(WHITE);
  dc.roundedRectangle({r.x+150, r.y+130, 100, 100}, 60, 4);
}

void draw_arc1(gx::DrawContext2D& dc, const gx::Rect& r)
{
  dc.color(WHITE);
  dc.arc({r.x + 200, r.y + 180}, 150, 0, 0, 32, 1);
}

void draw_arc2(gx::DrawContext2D& dc, const gx::Rect& r)
{
  dc.color(WHITE);
  dc.arc({r.x + 200, r.y + 180}, 150, 20, 270, 32, 16);
}

void draw_arc3(gx::DrawContext2D& dc, const gx::Rect& r)
{
  dc.arcShaded({r.x + 200, r.y + 180}, 150, 20, 270, 32, 32, BLACK, WHITE);
}

void draw_border1(gx::DrawContext2D& dc, const gx::Rect& r)
{
  dc.color(GRAY50);
  dc.border({r.x + 20, r.y + 30, 360, 300}, 4.0f);
}

void draw_border2(gx::DrawContext2D& dc, const gx::Rect& r)
{
  dc.hgradient(r.x + 20, BLACK, r.x + 380, WHITE);
  dc.border({r.x + 20, r.y + 30, 360, 300}, 8.0f);
}

void draw_border3(gx::DrawContext2D& dc, const gx::Rect& r)
{
  dc.vgradient(r.y + 30, BLACK, r.y + 330, WHITE);
  dc.border({r.x + 20, r.y + 30, 360, 300}, 8.0f);
}

void draw_border4(gx::DrawContext2D& dc, const gx::Rect& r)
{
  dc.borderShaded({r.x + 20, r.y + 30, 360, 300},
                  32.0f, WHITE, gx::packRGBA8(1,1,1,0), 0);
}

void draw_border5(gx::DrawContext2D& dc, const gx::Rect& r)
{
  dc.borderShaded({r.x + 20, r.y + 30, 360, 300},
                  32.0f, WHITE, gx::packRGBA8(1,1,1,0), RED);
}

void draw_rborder1(gx::DrawContext2D& dc, const gx::Rect& r)
{
  dc.color(GRAY50);
  dc.roundedBorder({r.x + 20, r.y + 30, 360, 300}, 40, 4, 4.0f);
}

void draw_rborder2(gx::DrawContext2D& dc, const gx::Rect& r)
{
  dc.hgradient(r.x + 20, WHITE, r.x + 380, BLACK);
  dc.roundedBorder({r.x + 20, r.y + 30, 360, 300}, 40, 4, 8.0f);
}

void draw_rborder3(gx::DrawContext2D& dc, const gx::Rect& r)
{
  dc.vgradient(r.y + 30, WHITE, r.y + 330, BLACK);
  dc.roundedBorder({r.x + 20, r.y + 30, 360, 300}, 40, 4, 8.0f);
}

void draw_rborder4(gx::DrawContext2D& dc, const gx::Rect& r)
{
  dc.roundedBorderShaded({r.x + 20, r.y + 30, 360, 300},
                         48, 4, 32.0f, WHITE, gx::packRGBA8(1,1,1,0), 0);
}

void draw_rborder5(gx::DrawContext2D& dc, const gx::Rect& r)
{
  dc.roundedBorderShaded({r.x + 20, r.y + 30, 360, 300},
                         48, 4, 32.0f, WHITE, gx::packRGBA8(1,1,1,0), RED);
}

void draw_lines1(gx::DrawContext2D& dc, const gx::Rect& r)
{
  const float x = r.x + .5f;
  const float y = r.y + .5f;
  const gx::Vec2 origin{x+20,y+30};

  dc.color(WHITE);
  //dc.lineWidth(.5f);
  dc.line(origin, gx::Vec2{x+379,y+30});
  dc.line(origin, gx::Vec2{x+379,y+180});
  dc.line(origin, gx::Vec2{x+379,y+329});
  dc.line(origin, gx::Vec2{x+200,y+329});
  dc.line(origin, gx::Vec2{x+20,y+329});
}

void draw_lines2(gx::DrawContext2D& dc, const gx::Rect& r)
{
  const float x = r.x + .5f;
  const float y = r.y + .5f;
  const gx::Vertex2C origin{x+20,y+30,RED};

  dc.line(origin, gx::Vertex2C{x+379,y+30,WHITE});
  dc.line(origin, gx::Vertex2C{x+379,y+180,WHITE});
  dc.line(origin, gx::Vertex2C{x+379,y+329,WHITE});
  dc.line(origin, gx::Vertex2C{x+200,y+329,WHITE});
  dc.line(origin, gx::Vertex2C{x+20,y+329,WHITE});
}

void draw_text1(gx::DrawContext2D& dc, const gx::Rect& r)
{
  gx::TextFormat tf{.font = &TheFont};
  tf.scale(2.0f);
  dc.color(WHITE);

  for (int i = 0; i < 5; ++i) {
    dc.text(tf, {r.x+60, r.y+55}, gx::Align::center_left, "  abc ABC 123");
    tf.rotate(gx::degToRad(22.5f));
  }
}

void draw_text2(gx::DrawContext2D& dc, const gx::Rect& r)
{
  const float cx = r.centerX();
  const float cy = r.centerY();

  dc.color(BLACK);
  dc.rectangle({r.x+4, r.y+24, r.w-8, r.h-24});
  dc.color(WHITE);
  dc.line(Vec2{r.x+4, cy}, Vec2{r.x+r.w-8, cy});
  dc.line(Vec2{cx, r.y+24}, Vec2{cx, r.y+r.h});

  gx::TextFormat tf{.font = &TheFont};
  tf.scale(2.0f);
  dc.color(RED);
  dc.text(tf, {cx,cy}, gx::Align::top_left, "ABC\nDEF");
  //dc.glyph(tf, {cx,cy}, gx::Align::top_left, 'A');
}

void draw_text3(gx::DrawContext2D& dc, const gx::Rect& r)
{
  const float cx = r.centerX();
  const float cy = r.centerY();

  dc.color(BLACK);
  dc.rectangle({r.x+4, r.y+24, r.w-8, r.h-24});
  dc.color(WHITE);
  dc.line(Vec2{r.x+4, cy}, Vec2{r.x+r.w-8, cy});
  dc.line(Vec2{cx, r.y+24}, Vec2{cx, r.y+r.h});

  gx::TextFormat tf{.font = &TheFont};
  tf.scale(2.0f);
  dc.color(RED);
  dc.text(tf, {cx,cy}, gx::Align::center, "ABC\nDEF");
  //dc.glyph(tf, {cx,cy}, gx::Align::center, 'A');
}

void draw_text4(gx::DrawContext2D& dc, const gx::Rect& r)
{
  const float cx = r.centerX();
  const float cy = r.centerY();

  dc.color(BLACK);
  dc.rectangle({r.x+4, r.y+24, r.w-8, r.h-24});
  dc.color(WHITE);
  dc.line(Vec2{r.x+4, cy}, Vec2{r.x+r.w-8, cy});
  dc.line(Vec2{cx, r.y+24}, Vec2{cx, r.y+r.h});

  gx::TextFormat tf{.font = &TheFont};
  tf.scale(2.0f);
  dc.color(RED);
  dc.text(tf, {cx,cy}, gx::Align::bottom_right, "ABC\nDEF");
  //dc.glyph(tf, {cx,cy}, gx::Align::bottom_right, 'A');
}


struct { const char* desc; void(*fn)(gx::DrawContext2D&,const gx::Rect&); }
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
  {"Text Alignment Top/Left", draw_text2},
  {"Text Alignment Center", draw_text3},
  {"Text Alignment Bottom/Right", draw_text4},
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

  const gx::TextFormat tf{.font = &TheFont};
  const int gfxCount = std::size(gfxData);
  int page = 0, gfxPerPage = 0, maxPage = 0;
  bool redraw = true, running = true;

  gx::DrawList dl;
  gx::DrawContext2D dc{dl};

  while (running) {
    // handle events
    gx::Window::pollEvents();
    const gx::EventState& es = win.eventState();

    if (es.closed()) { running = false; }
    if (es.resized()) { gfxPerPage = 0; redraw = true; }

    if (es.keyEvent()) {
      int newPage = page;
      for (const gx::KeyState& ks : es.keyStates) {
        if (!ks.pressCount && !ks.repeatCount) { continue; }

        switch (ks.key) {
          case gx::KEY_ESCAPE:
            running = false; break;
          case gx::KEY_LEFT: case gx::KEY_UP: case gx::KEY_PAGE_UP:
            --newPage; break;
          case gx::KEY_RIGHT: case gx::KEY_DOWN: case gx::KEY_PAGE_DOWN:
            ++newPage; break;
          case gx::KEY_F11:
            if (ks.pressCount) {
              gfxPerPage = 0;
              redraw = true;
              if (win.fullScreen()) {
                win.setSize(DEFAULT_WIDTH, DEFAULT_HEIGHT, false);
              } else {
                win.setSize(0, 0, true);
              }
            }
            break;
          default: break;
        }
      }

      if (newPage != page && newPage >= 0 && newPage <= maxPage) {
        page = newPage;
        redraw = true;
      }
    }

    // draw frame
    if (redraw) {
      const auto [width,height] = win.dimensions();
      if (gfxPerPage == 0) {
        page = 0;
        gfxPerPage = std::max(width / ITEM_WIDTH, 1) *
          std::max(height / ITEM_HEIGHT, 1);
        maxPage = (gfxCount - 1) / gfxPerPage;
      }

      const int start_gfx = page * gfxPerPage;
      const int end_gfx = std::min(start_gfx + gfxPerPage, gfxCount);
      dc.clearList();
      dc.clearView(.2f,.2f,.5f);

      // draw function & text split to reduce draw calls
      float x = 0, y = 0;
      for (int i = start_gfx; i < end_gfx; ++i) {
        gfxData[i].fn(dc, gx::Rect{x, y, ITEM_WIDTH, ITEM_HEIGHT});
        x += ITEM_WIDTH;
        if (x > float(width - ITEM_WIDTH)) { x = 0; y += ITEM_HEIGHT; }
      }

      x = y = 0;
      for (int i = start_gfx; i < end_gfx; ++i) {
        dc.color(WHITE);
        dc.text(tf, {x+(ITEM_WIDTH/2), y+6}, gx::Align::top_center,
                gfxData[i].desc);
        x += ITEM_WIDTH;
        if (x > float(width - ITEM_WIDTH)) { x = 0; y += ITEM_HEIGHT; }
      }

      if (maxPage > 0) {
        dc.color(WHITE);
        dc.text(tf, {float(width-10), float(height-1)},
                gx::Align::bottom_right,
                gx::concat("Page ", page+1, " of ", maxPage+1));
      }

      win.draw(dl);
      redraw = false;
    }

    win.renderFrame();
  }

  return 0;
}
