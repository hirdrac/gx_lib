//
// image_viewer.cc
// Copyright (C) 2024 Richard Bradley
//

// TODO: background loading of images after 1st image loaded
// TODO: multiple image display in fullscreen (horizontal/vertical)
// TODO: smooth scrolling when moving to next image
// TODO: zoom in/out at mouse point
// TODO: default starting zoom command line setting
// TODO: add support for font files (like font_viewer)

#include "gx/Image.hh"
#include "gx/Logger.hh"
#include "gx/Window.hh"
#include "gx/DrawContext.hh"
#include "gx/Print.hh"
#include "gx/StringUtil.hh"
#include "gx/CmdLineParser.hh"
#include <vector>

using gx::println;
using gx::println_err;


// globals
int winWidth = -1;
int winHeight = -1;

void setWinSize(gx::Window& win, const gx::Image& img)
{
  const int w = (winWidth > 0) ? winWidth : img.width();
  const int h = (winHeight > 0) ? winHeight : img.height();
  win.setSize(w, h, false);
}

struct Entry {
  std::string file;
  gx::Image img;
  gx::TextureHandle tex;
};

struct ImgSize { float width, height; };

[[nodiscard]] ImgSize calcSize(
  const gx::Window& win, const gx::Image& img, float scale)
{
  const auto [width,height] = win.dimensions();
  float iw = float(width);
  float ih = float(height);
  if (win.fullScreen()) {
    const float w_ratio = float(width) / float(img.width());
    const float h_ratio = float(height) / float(img.height());
    if (w_ratio > h_ratio) {
      iw = float(img.width()) * h_ratio;
    } else {
      ih = float(img.height()) * w_ratio;
    }
  }
  return {iw*scale, ih*scale};
}

int showUsage(const char* const* argv)
{
  println("Usage: ", argv[0], " [options] <image file(s)>");
  println("Options:");
  println("  --width=PIXELS             Set window width");
  println("  --height=PIXELS            Set window height");
  println("  --fullscreen               Start in fullscreen mode");
  println("  --filter=(linear|nearest)  Image filter setting");
  println("  -h,--help                  Show usage");
  return 0;
}

int errorUsage(const char* const* argv)
{
  println_err("Try '", argv[0], " --help' for more information.");
  return -1;
}

int main(int argc, char* argv[])
{
  if (argc < 2) {
    println_err("No image filenames specified");
    return errorUsage(argv);
  }

  gx::defaultLogger().disable();

  // command line processing
  bool startFullScreen = false;
  gx::FilterType magFilter = gx::FILTER_NEAREST;
  std::string val;

  std::vector<Entry> entries;
  for (gx::CmdLineParser p{argc, argv}; p; ++p) {
    if (p.option()) {
      if (p.option(0,"width", winWidth)) {
        if (winWidth <= 0) {
          println_err("ERROR: invalid width");
          return errorUsage(argv);
        }
      } else if (p.option(0,"height", winHeight)) {
        if (winHeight <= 0) {
          println_err("ERROR: invalid height");
          return errorUsage(argv);
        }
      } else if (p.option(0,"fullscreen")) {
        startFullScreen = true;
      } else if (p.option(0,"filter", val)) {
        if (val == "nearest") { magFilter = gx::FILTER_NEAREST; }
        else if (val == "linear") { magFilter = gx::FILTER_LINEAR; }
        else {
          println_err("ERROR: unknown filter type '", val, "'");
          return errorUsage(argv);
        }
      } else if (p.option('h',"help")) {
        return showUsage(argv);
      } else {
        println_err("ERROR: Bad option '", p.arg(), "'");
        return errorUsage(argv);
      }
    } else {
      // filename argument
      Entry e;
      p.get(e.file);
      if (!e.img.load(e.file)) {
        println_err("Can't load \"", e.file, "\"");
        continue;
      }
      entries.push_back(std::move(e));
    }
  }

  if (entries.empty()) {
    println_err("No images to display");
    return -1;
  }

  gx::Window win;
  if (startFullScreen) {
    win.setSize(0, 0, true);
  } else {
    setWinSize(win, entries[0].img);
  }

  if (!win.open(gx::WINDOW_RESIZABLE | gx::WINDOW_FIXED_ASPECT_RATIO)) {
    println_err("Failed to open window");
    return -1;
  }

  int zoom = 100;
  float imgScale = 1.0f;
  gx::Vec2 imgOffset;
  gx::Vec2 pressPos;
  const int lastNo = int(entries.size()) - 1;

  gx::TextureParams params;
  params.levels = 6;
  params.minFilter = gx::FILTER_LINEAR;
  params.magFilter = magFilter;
  params.mipFilter = gx::FILTER_LINEAR;

  gx::Renderer& ren = win.renderer();
  int entryNo = 0;
  for (Entry& e : entries) {
    e.tex = ren.newTexture(e.img, params);
  }

  gx::DrawList dl;
  gx::DrawContext dc{dl};
  bool redraw = true, running = true;
  constexpr float border = 8.0f;

  // main loop
  while (running) {
    // handle events
    gx::Window::pollEvents();
    const gx::EventState& es = win.eventState();

    if (es.events & gx::EVENT_SIZE) { redraw = true; }
    if (es.events & gx::EVENT_CLOSE) { running = false; }

    if (es.events & gx::EVENT_INPUT) {
      int no = entryNo;
      const bool last_entry = (no == lastNo);
      const bool first_entry = (no == 0);

      for (const auto& in : es.inputStates) {
        if (!in.pressCount && !in.repeatCount) { continue; }

        switch (in.value) {
          case gx::KEY_LEFT: case gx::KEY_BACKSPACE:
            if ((in.pressCount || !first_entry) && --no < 0) { no = lastNo; }
            break;
          case gx::KEY_RIGHT: case gx::KEY_SPACE:
            if ((in.pressCount || !last_entry) && ++no > lastNo) { no = 0; }
            break;
          case gx::KEY_HOME: no = 0; break;
          case gx::KEY_END: no = lastNo; break;

          case gx::KEY_ESCAPE: running = false; break;
          case gx::KEY_F11:
            if (in.pressCount) {
              redraw = true;
              zoom = 100;
              imgScale = 1.0f;
              imgOffset = {0,0};
              if (win.fullScreen()) {
                setWinSize(win, entries[std::size_t(entryNo)].img);
              } else {
                win.setSize(0, 0, true);
              }
            }
            break;
        }
      }

      if (no != entryNo) {
        imgOffset = {0,0};
        entryNo = no;

        const Entry& e = entries[std::size_t(entryNo)];
        if (!win.fullScreen()) { setWinSize(win, e.img); }
        win.setTitle(
          gx::concat(e.file, " - ", e.img.width(), 'x', e.img.height(),
                     'x', e.img.channels()));
        redraw = true;
      }
    }

    if (win.fullScreen()) {
      if (es.mouseScroll()) {
        const bool shiftHeld = es.mods & gx::MOD_SHIFT;
        const int scroll = static_cast<int>(es.scrollPt.y * (shiftHeld ? 8.0f : 1.0f));
        const int zoom2 = std::clamp(zoom + scroll, 20, 400);
        if (zoom != zoom2) {
          zoom = zoom2;
          imgScale = gx::sqr(float(std::max(zoom, 1)) / 100.f);
          // TODO: adjust imgOffset to zoom in at mouse point/screen center
          redraw = true;
        }
      }

      if (es.inputPress(gx::BUTTON_1)) {
        pressPos = es.mousePt;
      } else if (es.inputDrag(gx::BUTTON_1)) {
        const auto pt = es.mousePt;
        imgOffset += (pt - pressPos);
        pressPos = pt;
        redraw = true;
      }
    }

    // draw frame
    const Entry& e = entries[std::size_t(entryNo)];
    if (redraw) {
      const auto [width,height] = win.dimensions();
      const auto [iw,ih] = calcSize(win, e.img, imgScale);
      const float ix = std::floor((float(width) - iw) * .5f);
      const float iy = std::floor((float(height) - ih) * .5f);
      dc.clearList();
      dc.clearView(gx::BLACK);
      dc.color(gx::WHITE);
      dc.texture(e.tex);
      dc.rectangle({ix + imgOffset.x, iy + imgOffset.y, iw, ih}, {0,0}, {1,1});

      // multi-image horizontal display in fullscreen
      if (win.fullScreen() && iw < (float(width) - border)) {
        dc.color(gx::GRAY50);

        // display previous image(s)
        float prev_x = ix;
        for (int x = entryNo - 1; x >= 0; --x) {
          const Entry& e0 = entries[std::size_t(x)];
          const auto [iw0,ih0] = calcSize(win, e0.img, imgScale);
          const float ix0 = std::floor(prev_x - (iw0 + border)); prev_x = ix0;
          //if ((ix0+iw0) < 0) { break; }
          const float iy0 = std::floor((float(win.height()) - ih0) * .5f);
          dc.texture(e0.tex);
          dc.rectangle({ix0 + imgOffset.x, iy0 + imgOffset.y, iw0, ih0},
                       {0,0}, {1,1});
        }

        // display next image(s)
        prev_x = ix + iw + border;
        for (int x = entryNo + 1; x <= lastNo; ++x) {
          const Entry& e1 = entries[std::size_t(x)];
          const auto [iw1,ih1] = calcSize(win, e1.img, imgScale);
          const float ix1 = prev_x; prev_x += std::floor(iw1 + border);
          //if (ix1 > float(width)) { break; }
          const float iy1 = std::floor((float(win.height()) - ih1) * .5f);
          dc.texture(e1.tex);
          dc.rectangle({ix1 + imgOffset.x, iy1 + imgOffset.y, iw1, ih1},
                       {0,0}, {1,1});
        }
      }

      win.draw(dl);
      redraw = false;
    }

    win.renderFrame();
  }

  return 0;
}
