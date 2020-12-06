#include "gx/Window.hh"
#include "gx/Font.hh"
#include "gx/Gui.hh"
#include "gx/Print.hh"


constexpr int DEFAULT_WIDTH = 1280;
constexpr int DEFAULT_HEIGHT = 720;

int main(int argc, char* argv[])
{
  gx::Font fnt;
  if (!fnt.init("data/FreeSans.ttf", 24)) {
    gx::println_err("failed to load font");
    return -1;
  }

  gx::GuiTheme theme;
  theme.baseFont = &fnt;

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
      gx::guiButton(gx::ALIGN_RIGHT, "[QUIT]", 99)));
  gui1.layout(theme, 100, 100, gx::ALIGN_TOP_LEFT);

  gx::Gui gui2(
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
        gx::guiMenuItem("About", 4))));
  gui2.layout(theme, 0, 0, gx::ALIGN_TOP_LEFT);

  gx::Gui gui3(
    gx::guiVFrame(
      gx::guiHFrame(
        gx::guiLabel("R"), gx::guiEntry(3.5f,3,1),
        gx::guiLabel(" G"), gx::guiEntry(3.5f,3,2),
        gx::guiLabel(" B"), gx::guiEntry(3.5f,3,3)),
      gx::guiHLine(),
      gx::guiEntry(16.0f,100,10)));
  gui3.layout(theme, 100, 300, gx::ALIGN_TOP_LEFT);

  gx::Window win;
  win.setTitle("GUI demo");
  win.setSize(DEFAULT_WIDTH, DEFAULT_HEIGHT, false);
  if (!win.open()) {
    gx::println_err("failed to open window");
    return -1;
  }

  win.setBGColor(.1,.3,.1);
  fnt.makeAtlas(win);
  bool running = true;
  bool needRedraw = true;

  while (running) {
    // draw frame
    if (win.resized() || needRedraw) {
      // something on screen changed - recreate GL buffers
      win.clearFrame();
      win.draw(gui1);
      win.draw(gui2);
      win.draw(gui3);
      needRedraw = false;
    }
    win.renderFrame();

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

    // update gui(s)
    gui1.update(win);
    needRedraw |= gui1.needRedraw();
    if (gui1.releasedID() > 0) {
      gx::println("GUI1 released:", gui1.releasedID());
      if (gui1.releasedID() == 99) { running = false; }
    }

    gui2.update(win);
    needRedraw |= gui2.needRedraw();
    if (gui2.releasedID() > 0) {
      gx::println("GUI2 released:", gui2.releasedID());
      if (gui2.releasedID() == 99) { running = false; }
    }

    gui3.update(win);
    needRedraw |= gui3.needRedraw();
    // FINISH - gui3 event handling
    //gx::println("time ", win.pollTime());
  }

  return 0;
}
