//
// image_viewer.cc
// Copyright (C) 2023 Richard Bradley
//

// TODO: background loading of images after 1st image loaded
// TODO: multiple image display in fullscreen (horizontal/vertical)
// TODO: smooth scrolling when moving to next image
// TODO: don't scroll if all images can fit on screen?
// TODO: option to enable full logging

#include "gx/Image.hh"
#include "gx/Logger.hh"
#include "gx/Window.hh"
#include "gx/Renderer.hh"
#include "gx/DrawLayer.hh"
#include "gx/DrawContext.hh"
#include "gx/Texture.hh"
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
  gx::Texture tex;
};

struct ImgSize { float width, height; };

ImgSize calcSize(const gx::Window& win, const gx::Image& img, float scale)
{
  float iw = float(win.width());
  float ih = float(win.height());
  if (win.fullScreen()) {
    const float w_ratio = float(win.width()) / float(img.width());
    const float h_ratio = float(win.height()) / float(img.height());
    if (w_ratio > h_ratio) {
      iw = float(img.width()) * h_ratio;
    } else {
      ih = float(img.height()) * w_ratio;
    }
  }
  return {iw*scale, ih*scale};
}

int showUsage(char** argv)
{
  println("Usage: ", argv[0], " [options] <image file(s)>");
  println("Options:");
  println("  --width       Set window width");
  println("  --height      Set window height");
  println("  --fullscreen  Start in fullscreen mode");
  println("  -h,--help     Show usage");
  return 0;
}

int errorUsage(char** argv)
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
  std::vector<Entry> entries;
  for (gx::CmdLineParser p{argc, argv}; p; ++p) {
    if (p.option()) {
      if (p.option(0,"width",winWidth)) {
        // noop
      } else if (p.option(0,"height",winHeight)) {
        // noop
      } else if (p.option(0,"fullscreen")) {
        startFullScreen = true;
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
  gx::Vec2 imgOffset{gx::INIT_ZERO};
  gx::Vec2 pressPos{gx::INIT_ZERO};
  const int lastNo = int(entries.size()) - 1;

  gx::Renderer& ren = win.renderer();

  int entryNo = 0;
  for (Entry& e : entries) {
    e.tex.init(ren, e.img, 4, gx::FILTER_LINEAR, gx::FILTER_NEAREST);
  }

  gx::DrawLayer dl;
  dl.setBGColor(gx::BLACK);

  gx::DrawContext dc{dl};
  bool refresh = true;
  constexpr float border = 8.0f;

  // main loop
  for (;;) {
    const Entry& e = entries[std::size_t(entryNo)];
    if (win.resized() || refresh) {
      if (refresh) {
        win.setTitle(
          gx::concat(e.file, " - ", e.img.width(), 'x', e.img.height(),
                     'x', e.img.channels()));
        refresh = false;
      }

      const auto [iw,ih] = calcSize(win, e.img, imgScale);
      const float ix = std::floor((float(win.width()) - iw) * .5f);
      const float iy = std::floor((float(win.height()) - ih) * .5f);
      dc.clear();
      dc.color(gx::WHITE);
      dc.texture(e.tex);
      dc.rectangle(ix + imgOffset.x, iy + imgOffset.y, iw, ih, {0,0}, {1,1});

      // multi-image horizontal display in fullscreen
      if (win.fullScreen() && iw < (float(win.width()) - border)) {
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
          dc.rectangle(ix0 + imgOffset.x, iy0 + imgOffset.y,
                       iw0, ih0, {0,0}, {1,1});
        }

        // display next image(s)
        prev_x = ix + iw + border;
        for (int x = entryNo + 1; x <= lastNo; ++x) {
          const Entry& e1 = entries[std::size_t(x)];
          const auto [iw1,ih1] = calcSize(win, e1.img, imgScale);
          const float ix1 = prev_x; prev_x += std::floor(iw1 + border);
          //if (ix1 > float(win.width())) { break; }
          const float iy1 = std::floor((float(win.height()) - ih1) * .5f);
          dc.texture(e1.tex);
          dc.rectangle(ix1 + imgOffset.x, iy1 + imgOffset.y,
                       iw1, ih1, {0,0}, {1,1});
        }
      }

      ren.draw(win.width(), win.height(), {&dl});
    }

    ren.renderFrame();

    gx::Window::pollEvents();
    if (win.closed() || win.keyPressCount(gx::KEY_ESCAPE, true)) { break; }
    if (win.keyPressCount(gx::KEY_F11, false)) {
      zoom = 100;
      imgScale = 1.0f;
      imgOffset = {0,0};
      if (win.fullScreen()) {
        setWinSize(win, e.img);
      } else {
        win.setSize(0, 0, true);
      }
    }

    int no = entryNo;
    const bool last_entry = (no == lastNo);
    const bool first_entry = (no == 0);
    if (win.keyPressCount(gx::KEY_LEFT, !first_entry)
        || win.keyPressCount(gx::KEY_BACKSPACE, !first_entry)) {
      if (--no < 0) { no = lastNo; }
    }
    if (win.keyPressCount(gx::KEY_RIGHT, !last_entry)
        || win.keyPressCount(gx::KEY_SPACE, !last_entry)) {
      if (++no > lastNo) { no = 0; }
    }
    if (win.keyPressCount(gx::KEY_HOME, false)) { no = 0; }
    if (win.keyPressCount(gx::KEY_END, false)) { no = lastNo; }

    if (win.fullScreen()) {
      if (win.events() & gx::EVENT_MOUSE_SCROLL) {
        const int zoom2 = std::clamp(zoom + int(win.scrollY()), 20, 400);
        if (zoom != zoom2) {
          zoom = zoom2;
          imgScale = gx::sqr(float(std::max(zoom, 1)) / 100.f);
          // TODO: adjust imgOffset to zoom in at mouse point/screen center
          refresh = true;
        }
      }

      if (win.buttonPress(gx::BUTTON1)) {
        pressPos = win.mousePt();
      } else if (win.buttonDrag(gx::BUTTON1)) {
        imgOffset += (win.mousePt() - pressPos);
        pressPos = win.mousePt();
        refresh = true;
      }
    }

    if (no != entryNo) {
      imgOffset = {0,0};
      entryNo = no;
      if (!win.fullScreen()) {
        const Entry& e2 = entries[std::size_t(entryNo)];
        setWinSize(win, e2.img);
      }
      refresh = true;
    }
  }

  return 0;
}
