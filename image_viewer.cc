//
// image_viewer.cc
// Copyright (C) 2021 Richard Bradley
//

#include "gx/Image.hh"
#include "gx/Logger.hh"
#include "gx/Window.hh"
#include "gx/Renderer.hh"
#include "gx/DrawList.hh"
#include "gx/DrawContext.hh"
#include "gx/Texture.hh"
#include "gx/Print.hh"
#include "gx/StringUtil.hh"
#include <vector>


// TODO
// - fullscreen zoom w/ mouse wheel
// - fixed aspect ratio for window resize
// - mouse drag of image when zoomed
// - multiple image support
//   - background loading after 1st image loaded
//   - multiple image display in fullscreen (horizontal/vertical)
//   - smooth scrolling when moving to next image
//   - don't scroll if all images can fit on screen?
// - make texture mag filter configurable? ('smooth' mode would use linear)

struct Entry
{
  std::string file;
  gx::Image img;
  gx::Texture tex;
};

std::pair<int,int> calcSize(const gx::Window& win, const gx::Image& img)
{
  int iw = win.width();
  int ih = win.height();
  if (win.fullScreen()) {
    float w_ratio = float(win.width()) / float(img.width());
    float h_ratio = float(win.height()) / float(img.height());
    if (w_ratio > h_ratio) {
      iw = int(float(img.width()) * h_ratio);
    } else {
      ih = int(float(img.height()) * w_ratio);
    }
  }
  return std::pair(iw,ih);
}

int main(int argc, char* argv[])
{
  if (argc < 2) {
    gx::println_err("Usage: ", argv[0], " <image file(s)>");
    return -1;
  }

  gx::defaultLogger().disable();

  std::vector<Entry> entries;
  for (int i = 1; i < argc; ++i) {
    Entry e;
    e.file = argv[i];
    if (!e.img.init(e.file)) {
      gx::println_err("Can't load \"", e.file, "\"");
      continue;
    }
    entries.push_back(std::move(e));
  }

  if (entries.empty()) {
    gx::println_err("No images to display");
    return -1;
  }

  const int lastNo = int(entries.size()) - 1;
  gx::Window win;
  win.setSize(entries[0].img.width(), entries[0].img.height(), false);
  if (!win.open()) {
    gx::println_err("failed to open window");
    return -1;
  }

  int entryNo = 0;
  for (Entry& e : entries) {
    e.tex.init(win, e.img, 3, gx::FILTER_LINEAR, gx::FILTER_NEAREST);
  }

  gx::Renderer& ren = win.renderer();
  ren.setBGColor(gx::BLACK);
  gx::DrawList dl;
  gx::DrawContext dc(dl);
  bool refresh = true;

  for (;;) {
    Entry& e = entries[entryNo];
    if (win.resized() || refresh) {
      win.setTitle(gx::concat(e.file, " - ", e.img.width(), 'x', e.img.height(),
                              'x', e.img.channels()));
      auto [iw,ih] = calcSize(win, e.img);
      int ix = int(float(win.width() - iw) / 2.0f);
      int iy = int(float(win.height() - ih) / 2.0f);
      dc.clear();
      dc.color(gx::WHITE);
      dc.texture(e.tex);
      dc.rectangle(float(ix), float(iy), float(iw), float(ih),
                   {0,0}, {1,1});
      dc.color(gx::GRAY50);

      // multi-image horizontal display in fullscreen
      constexpr int border = 8;
      if (iw < (win.width() - border)) {
        // display previous image(s)
        int prev_x = ix;
        for (int x = entryNo - 1; x >= 0; --x) {
          Entry& e0 = entries[x];
          auto [iw0,ih0] = calcSize(win, e0.img);
          int ix0 = prev_x - (iw0 + border); prev_x = ix0;
          if ((ix0+iw0) < 0) { break; }
          int iy0 = int(float(win.height() - ih0) / 2.0f);
          dc.texture(e0.tex);
          dc.rectangle(float(ix0), float(iy0), float(iw0), float(ih0),
                       {0,0}, {1,1});
        }

        // display next image(s)
        prev_x = ix + iw + border;
        for (int x = entryNo + 1; x <= lastNo; ++x) {
          Entry& e1 = entries[x];
          auto [iw1,ih1] = calcSize(win, e1.img);
          int ix1 = prev_x; prev_x += iw1 + border;
          if (ix1 > win.width()) { break; }
          int iy1 = int(float(win.height() - ih1) / 2.0f);
          dc.texture(e1.tex);
          dc.rectangle(float(ix1), float(iy1), float(iw1), float(ih1),
                       {0,0}, {1,1});
        }
      }

      ren.clearFrame(win.width(), win.height());
      ren.draw(dl);
      refresh = false;
    }

    ren.renderFrame();

    win.pollEvents();
    if (win.closed() || win.keyPressCount(gx::KEY_ESCAPE, true)) { break; }
    if (win.keyPressCount(gx::KEY_F11, false)) {
      if (win.fullScreen()) {
        win.setSize(e.img.width(), e.img.height(), false);
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

    if (no != entryNo) {
      entryNo = no;
      if (!win.fullScreen()) {
        Entry& e2 = entries[std::size_t(entryNo)];
        win.setSize(e2.img.width(), e2.img.height(), false);
      }
      refresh = true;
    }
  }

  return 0;
}
