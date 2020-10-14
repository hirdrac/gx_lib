#include "gx/Window.hh"
#include "gx/Font.hh"
#include "gx/Gui.hh"
#include "gx/print.hh"


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

  gx::Gui gui(
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

  gui.layout(theme, 100, 100, gx::ALIGN_TOP_LEFT);

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
      win.draw(gui);
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

    // update gui
    gui.update(win);
    needRedraw |= gui.needRedraw();
    //if (gui.buttonPressed()) { gx::println("pressed:", gui.buttonPressed()); }
    //if (gui.buttonHeld()) { gx::println("held:", gui.buttonHeld()); }
    if (gui.buttonReleased()) { gx::println("released:", gui.buttonReleased()); }
    if (gui.buttonReleased() == 99) {
      running = false;
    }
  }

  return 0;
}
