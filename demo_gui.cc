//
// demo_gui.cc
// Copyright (C) 2021 Richard Bradley
//

#include "gx/Window.hh"
#include "gx/Renderer.hh"
#include "gx/Font.hh"
#include "gx/Gui.hh"
#include "gx/Print.hh"


constexpr int DEFAULT_WIDTH = 1280;
constexpr int DEFAULT_HEIGHT = 720;
constexpr int FONT_SIZE = 24;

// font data from VariableWidthFontData.cc
extern unsigned char VariableWidthFontData[];
extern unsigned long VariableWidthFontDataSize;


int main(int argc, char* argv[])
{
  gx::Font fnt;
  if (!fnt.loadFromMemory(VariableWidthFontData, VariableWidthFontDataSize, FONT_SIZE)) {
    gx::println_err("failed to load font");
    return -1;
  }

  gx::GuiTheme theme{&fnt};

  gx::Gui gui1(
    gx::guiVFrame(
      gx::guiLabel(gx::ALIGN_HCENTER, "BUTTON LIST"),
      gx::guiHLine(),
      gx::guiHFrame(
        gx::guiButton("B1\nline 2", 1),
        gx::guiButton(gx::ALIGN_BOTTOM, "B2", 2),
        gx::guiButton(gx::ALIGN_VCENTER, "B3", 3),
        gx::guiButton("B4", 4),
        gx::guiVLine(),
        gx::guiButton("B5", 5)),
      gx::guiHFrame(
        gx::guiButtonPress("PRESS", 77),
        gx::guiButtonHold("HOLD", 88)),
      gx::guiButton(gx::ALIGN_RIGHT, "[QUIT]", 99)));
  gui1.layout(theme, 60, 80, gx::ALIGN_TOP_LEFT);

  gx::Gui gui2(
    gx::guiVFrame(
      gx::guiHFrame(
        gx::guiMenu(
          "File",
          gx::guiMenuItem("Open...", 1),
          gx::guiMenuItem("Save...", 2),
          gx::guiHLine(),
          gx::guiMenuItem("Quit", 99)),
        gx::guiMenu(
          "Help",
          gx::guiMenuItem("Manual", 3),
          gx::guiMenuItem("About", 4)))));
  gui2.layout(theme, 0, 0, gx::ALIGN_TOP_LEFT);

  gx::Gui gui3(
    gx::guiVFrame(
      gx::guiHFrame(
        gx::guiLabel(gx::ALIGN_VCENTER, "R"), gx::guiCardinalEntry(3.0f,3,1),
        gx::guiLabel(gx::ALIGN_VCENTER, " G"), gx::guiCardinalEntry(3.0f,3,2),
        gx::guiLabel(gx::ALIGN_VCENTER, " B"), gx::guiCardinalEntry(3.0f,3,3)),
      gx::guiHLine(),
      gx::guiTextEntry(16.0f,100,10)));
  gui3.layout(theme, 60, 300, gx::ALIGN_TOP_LEFT);

  gx::Window win;
  win.setTitle("GUI demo");
  win.setSize(DEFAULT_WIDTH, DEFAULT_HEIGHT, false);
  if (!win.open()) {
    gx::println_err("failed to open window");
    return -1;
  }

  gx::Renderer& ren = win.renderer();
  ren.setBGColor(.1f,.3f,.1f);
  fnt.makeAtlas(ren);
  bool running = true;
  bool needRedraw = true;

  while (running) {
    // draw frame
    if (win.resized() || needRedraw) {
      // something on screen changed - recreate GL buffers
      ren.clearFrame(win.width(), win.height());
      ren.draw(gui2);
      ren.draw(gui1);
      ren.draw(gui3);
      needRedraw = false;
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

    // update gui(s)
    gui1.update(win);
    needRedraw |= gui1.needRedraw();
    if (gui1.eventID() > 0) {
      gx::println_err("GUI1 event:", gui1.eventID());
      if (gui1.eventID() == 99) { running = false; }
    }

    gui2.update(win);
    needRedraw |= gui2.needRedraw();
    if (gui2.eventID() > 0) {
      gx::println_err("GUI2 event:", gui2.eventID());
      if (gui2.eventID() == 99) { running = false; }
    }

    gui3.update(win);
    needRedraw |= gui3.needRedraw();
    if (gui3.eventID() > 0) {
      gx::println_err("GUI3 event:", gui3.eventID(), ": ",
                      gui3.getText(gui3.eventID()));
    }

    //gx::println_err("time ", gx::Window::lastPollTime());
  }

  return 0;
}
