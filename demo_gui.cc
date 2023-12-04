//
// demo_gui.cc
// Copyright (C) 2023 Richard Bradley
//

#include "gx/Window.hh"
#include "gx/Font.hh"
#include "gx/GuiBuilder.hh"
#include "gx/Gui.hh"
#include "gx/Print.hh"

using gx::print_err;
using gx::println_err;


constexpr int DEFAULT_WIDTH = 1280;
constexpr int DEFAULT_HEIGHT = 720;
constexpr int FONT_SIZE = 24;

// font data from VariableWidthFontData.cc
extern unsigned char VariableWidthFontData[];
extern unsigned long VariableWidthFontDataSize;


int main(int argc, char* argv[])
{
  gx::Font fnt{FONT_SIZE};
  if (!fnt.loadFromMemory(VariableWidthFontData, VariableWidthFontDataSize)) {
    println_err("failed to load font");
    return -1;
  }

  gx::GuiTheme theme{&fnt};

  // button demo
  gx::Gui gui;
  gui.newPanel(
    theme, 60, 80, gx::ALIGN_TOP_LEFT, gx::PANEL_FLOATING,
    gx::guiVFrame(
      gx::guiMargin(gx::guiTitleBar("BUTTONS"), 0,0,0,8),
      gx::guiHFrame(
        gx::guiButton(1, "B1\nline 2"),
        gx::guiButton(2, gx::ALIGN_BOTTOM, "B2"),
        gx::guiButton(3, gx::ALIGN_VCENTER, "B3"),
        gx::guiButton(4, "B4"),
        gx::guiVLine(),
        gx::guiButton(5, gx::ALIGN_VJUSTIFY, "B5")),
      gx::guiLabel(gx::ALIGN_CENTER, "\nRepeating Buttons"),
      gx::guiHFrame(
        gx::ALIGN_CENTER,
        gx::guiButtonPress(77, -1, "ONCE"),
        gx::guiButtonPress(78, 400000, "SLOW"),
        gx::guiButtonPress(79, 1, "FAST")),
      gx::guiHLine(),
      gx::guiButton(99, gx::ALIGN_RIGHT, " QUIT ")));

  // pull-down menu demo
  gui.newPanel(
    theme, 0, 0, gx::ALIGN_TOP_LEFT, 0,
    gx::guiHFrame(
      gx::guiMenu(
        11, "File",
        gx::guiMenuItem(11, "Open..."),
        gx::guiMenuItem(12, "Save..."),
        gx::guiHLine(),
        gx::guiMenuItem(99, "Quit")),
      gx::guiMenu(
        12, "Help",
        gx::guiMenuItem(13, "Manual"),
        gx::guiMenuItem(14, "About"),
        gx::guiSubMenu(
          "sub1",
          gx::guiMenuItem(15, "item 1"),
          gx::guiMenuItem(16, "item 2"),
          gx::guiMenuItem(17, "item 3")),
        gx::guiSubMenu(
          "sub2 long name",
          gx::guiMenuItem(18, "item 4"),
          gx::guiMenuItem(19, "item 5"),
          gx::guiMenuItem(20, "item 6")) )));

  // text entry demo
  gui.newPanel(
    theme, 60, 400, gx::ALIGN_TOP_LEFT, gx::PANEL_FLOATING,
    gx::guiHFrame(
      gx::guiMargin(gx::guiVTitleBar("ENTRIES"), 0,0,4,0),
      gx::guiVFrame(
        gx::guiHFrame(
          gx::guiLabel(gx::ALIGN_CENTER_LEFT, " R"),
          gx::guiCardinalEntry(31, 3.0f,3),
          gx::guiLabel(gx::ALIGN_CENTER_LEFT, " G"),
          gx::guiCardinalEntry(32, 3.0f,3),
          gx::guiLabel(gx::ALIGN_CENTER_LEFT, " B"),
          gx::guiCardinalEntry(33, 3.0f,3)),
        gx::guiTextEntry(34, 18.0f,100),
        gx::guiTextEntry(35, 18.0f,100,gx::ALIGN_CENTER),
        gx::guiTextEntry(36, 18.0f,100,gx::ALIGN_RIGHT),
        gx::guiIntegerEntry(37, 10.0f, 20),
        gx::guiFloatEntry(38, 10.0f, 20) )));

  // checkbox demo
  gui.newPanel(
    theme, 400, 80, gx::ALIGN_TOP_LEFT, gx::PANEL_FLOATING,
    gx::guiVFrame(
      gx::guiMargin(gx::guiTitleBar(), 0,0,0,8),
      gx::guiCheckbox(51, true,  "Option 1"),
      gx::guiCheckbox(52, false, "Option 2"),
      gx::guiHLine(),
      gx::guiCheckbox(53, false, "Option 3\n(line 2)"),
      gx::guiHLine(),
      gx::guiCheckbox(54, false, "Option 4\n(line 2)\n(line 3)")));

  // list select demo
  gui.newPanel(
    theme, 600, 80, gx::ALIGN_TOP_LEFT, gx::PANEL_FLOATING,
    gx::guiVFrame(
      gx::guiMargin(gx::guiTitleBar("LIST SELECTORS"), 0,0,0,8),
      gx::guiHFrame(
        gx::guiListSelect(
          60, gx::ALIGN_JUSTIFY,
          gx::guiListSelectItem(1, "item 1"),
          gx::guiListSelectItem(2, "item two"),
          gx::guiListSelectItem(3, "item three")),
        gx::guiListSelect(
          61, gx::ALIGN_JUSTIFY,
          gx::guiListSelectItem(4, "item four"),
          gx::guiListSelectItem(5, "item 5"),
          gx::guiListSelectItem(6, "item six")) )));

  // window setup
  gx::Window win;
  win.setTitle("GUI demo");
  win.setSize(DEFAULT_WIDTH, DEFAULT_HEIGHT, false);
  if (!win.open()) {
    println_err("failed to open window");
    return -1;
  }

  fnt.makeAtlas(win);
  bool running = true;
  bool needRedraw = true;

  gui.setBGColor(.1f,.3f,.1f);

  // **** MAIN LOOP ****
  while (running) {
    // draw frame
    if (win.resized() || needRedraw) {
      // something on screen changed - recreate GL buffers
      win.draw(&gui.drawList());
      needRedraw = false;
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

    // update gui
    needRedraw |= gui.update(win);
    if (gui.event()) {
      print_err("GUI event:", gui.event().eid);
      switch (gui.event().type) {
        case gx::GUI_ENTRY:
          println_err("\ttext:\"", gui.eventText(), "\"");
          break;
        case gx::GUI_LISTSELECT:
          println_err("\titem_no:", gui.event().item_no);
          break;
        case gx::GUI_MENU:
          println_err("\tmenu_item_no:", gui.event().item_no);
          // check quit menu item
          if (gui.event().item_no == 99) { running = false; }
          break;
        case gx::GUI_CHECKBOX:
          println_err("\tset:", gui.eventBool());
          break;
        default:
          println_err();
          break;
      }
      // check quit button
      if (gui.event().eid == 99) { running = false; }
    }

    //println_err("time ", gx::Window::lastPollTime());
  }

  return 0;
}
