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

  // button demo
  gx::Gui gui1{
    gx::guiVFrame(
      gx::guiLabel(gx::ALIGN_HCENTER, "BUTTON LIST"),
      gx::guiHLine(),
      gx::guiHFrame(
        gx::guiButton(1, "B1\nline 2"),
        gx::guiButton(2, gx::ALIGN_BOTTOM, "B2"),
        gx::guiButton(3, gx::ALIGN_VCENTER, "B3"),
        gx::guiButton(4, "B4"),
        gx::guiVLine(),
        gx::guiButton(5, gx::ALIGN_VJUSTIFY, "B5")),
      gx::guiHFrame(
        gx::guiButtonPress(77, "PRESS"),
        gx::guiButtonHold(88, "HOLD")),
      gx::guiHLine(),
      gx::guiButton(99, gx::ALIGN_RIGHT, "[QUIT]"))};
  gui1.layout(theme, 60, 80, gx::ALIGN_TOP_LEFT);

  // pull-down menu demo
  gx::Gui gui2{
    gx::guiHFrame(
      gx::guiMenu(
        "File",
        gx::guiMenuItem(1, "Open..."),
        gx::guiMenuItem(2, "Save..."),
        gx::guiHLine(),
        gx::guiMenuItem(99, "Quit")),
      gx::guiMenu(
        "Help",
        gx::guiMenuItem(3, "Manual"),
        gx::guiMenuItem(4, "About")),
      gx::guiVLine(),
      gx::guiCheckbox(21, true, "click me!"))
  };
  gui2.layout(theme, 0, 0, gx::ALIGN_TOP_LEFT);

  // text entry demo
  gx::Gui gui3{
    gx::guiVFrame(
      gx::guiHFrame(
        gx::guiLabel(gx::ALIGN_CENTER_LEFT, " R"),
        gx::guiCardinalEntry(1, 3.0f,3),
        gx::guiLabel(gx::ALIGN_CENTER_LEFT, " G"),
        gx::guiCardinalEntry(2, 3.0f,3),
        gx::guiLabel(gx::ALIGN_CENTER_LEFT, " B"),
        gx::guiCardinalEntry(3, 3.0f,3)),
      //gx::guiHLine(),
      gx::guiTextEntry(10, 16.0f,100))};
  gui3.layout(theme, 60, 320, gx::ALIGN_TOP_LEFT);

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

  // **** MAIN LOOP ****
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
