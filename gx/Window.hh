//
// gx/Window.hh
// Copyright (C) 2021 Richard Bradley
//
// encapsulation of core graphics output & user input
//

// TODO - settings for resizable,decorated,fixedAspectRatio
// TODO - setting for min/max width/height on resizable windows
// TODO - frame stats (draw calls, buffer size)

#pragma once
#include "Color.hh"
#include "DrawList.hh"
#include <string_view>
#include <string>
#include <vector>
#include <memory>


struct GLFWwindow;

namespace gx {
  // event mask
  enum {
    // window events
    EVENT_CLOSE        = 1<<0,
    EVENT_SIZE         = 1<<1,
    EVENT_ICONIFY      = 1<<2,
    EVENT_FOCUS        = 1<<3,

    // key events
    EVENT_KEY          = 1<<4,
    EVENT_CHAR         = 1<<5,

    // mouse events
    EVENT_MOUSE_ENTER  = 1<<6,
    EVENT_MOUSE_MOVE   = 1<<7,
    EVENT_MOUSE_SCROLL = 1<<8,

    EVENT_MOUSE_BUTTON1 = 1<<11, // Left
    EVENT_MOUSE_BUTTON2 = 1<<12, // Right
    EVENT_MOUSE_BUTTON3 = 1<<13, // Middle
    EVENT_MOUSE_BUTTON4 = 1<<14,
    EVENT_MOUSE_BUTTON5 = 1<<15,
    EVENT_MOUSE_BUTTON6 = 1<<16,
    EVENT_MOUSE_BUTTON7 = 1<<17,
    EVENT_MOUSE_BUTTON8 = 1<<18,
    EVENT_MOUSE_ANY_BUTTON = (0b11111111<<11),
  };

  // event values
  enum {
    BUTTON1 = 1<<0, BUTTON2 = 1<<1, BUTTON3 = 1<<2, BUTTON4 = 1<<3,
    BUTTON5 = 1<<4, BUTTON6 = 1<<5, BUTTON7 = 1<<6, BUTTON8 = 1<<7
  };

  enum { MOD_SHIFT = 1, MOD_CONTROL = 2, MOD_ALT = 4, MOD_SUPER = 8 };

  enum {
    // special key values (adapted from glfw3.h)
    KEY_UNKNOWN = -1,
    KEY_SPACE = 32, KEY_APOSTROPHE = 39, KEY_COMMA = 44,
    KEY_MINUS = 45, KEY_PERIOD = 46, KEY_FWD_SLASH = 47,
    KEY_0 = 48, KEY_1 = 49, KEY_2 = 50, KEY_3 = 51, KEY_4 = 52,
    KEY_5 = 53, KEY_6 = 54, KEY_7 = 55, KEY_8 = 56, KEY_9 = 57,
    KEY_SEMICOLON = 59, KEY_EQUAL = 61,
    KEY_A = 65, KEY_B = 66, KEY_C = 67, KEY_D = 68, KEY_E = 69, KEY_F = 70,
    KEY_G = 71, KEY_H = 72, KEY_I = 73, KEY_J = 74, KEY_K = 75, KEY_L = 76,
    KEY_M = 77, KEY_N = 78, KEY_O = 79, KEY_P = 80, KEY_Q = 81, KEY_R = 82,
    KEY_S = 83, KEY_T = 84, KEY_U = 85, KEY_V = 86, KEY_W = 87, KEY_X = 88,
    KEY_Y = 89, KEY_Z = 90,
    KEY_LBRACKET = 91, KEY_BACK_SLASH = 92, KEY_RBRACKET = 93, KEY_TILDA = 96,
    KEY_WORLD_1 = 161, KEY_WORLD_2 = 162,
    KEY_ESCAPE = 256, KEY_ENTER = 257, KEY_TAB = 258,
    KEY_BACKSPACE = 259, KEY_INSERT = 260, KEY_DELETE = 261,
    KEY_RIGHT = 262, KEY_LEFT = 263, KEY_DOWN = 264, KEY_UP = 265,
    KEY_PAGE_UP = 266, KEY_PAGE_DOWN = 267, KEY_HOME = 268, KEY_END = 269,
    KEY_CAPS_LOCK = 280, KEY_SCROLL_LOCK = 281, KEY_NUM_LOCK = 282,
    KEY_PRINT_SCREEN = 283, KEY_PAUSE = 284,
    KEY_F1 = 290, KEY_F2 = 291, KEY_F3 = 292, KEY_F4 = 293, KEY_F5 = 294,
    KEY_F6 = 295, KEY_F7 = 296, KEY_F8 = 297, KEY_F9 = 298, KEY_F10 = 299,
    KEY_F11 = 300, KEY_F12 = 301, KEY_F13 = 302, KEY_F14 = 303, KEY_F15 = 304,
    KEY_F16 = 305, KEY_F17 = 306, KEY_F18 = 307, KEY_F19 = 308, KEY_F20 = 309,
    KEY_F21 = 310, KEY_F22 = 311, KEY_F23 = 312, KEY_F24 = 313, KEY_F25 = 314,
    KEY_KP_0 = 320, KEY_KP_1 = 321, KEY_KP_2 = 322, KEY_KP_3 = 323,
    KEY_KP_4 = 324, KEY_KP_5 = 325, KEY_KP_6 = 326, KEY_KP_7 = 327,
    KEY_KP_8 = 328, KEY_KP_9 = 329,
    KEY_KP_DECIMAL = 330, KEY_KP_DIVIDE = 331, KEY_KP_MULTIPLY = 332,
    KEY_KP_SUBTRACT = 333, KEY_KP_ADD = 334,
    KEY_KP_ENTER = 335, KEY_KP_EQUAL = 336,
    KEY_LSHIFT = 340, KEY_LCONTROL = 341,
    KEY_LALT = 342, KEY_LSUPER = 343,
    KEY_RSHIFT = 344, KEY_RCONTROL = 345,
    KEY_RALT = 346, KEY_RSUPER = 347, KEY_MENU = 348
  };

  struct KeyState {
    int16_t key;         // key value
    int16_t pressCount;  // number of press events since pollEvents
    int16_t repeatCount; // number of repeat events since pollEvents
    bool    pressed;     // key is currently pressed
  };

  struct CharInfo {
    // unicode value
    uint32_t codepoint;

    // set if non-printable key (codepoint is zero)
    int16_t key;
    uint8_t mods;
    bool repeat;
  };

  // Setting values
  enum {
    WINDOW_DECORATED = 1, // ignored for fullscreen
    WINDOW_RESIZABLE = 2, // decorated implied
    WINDOW_DEBUG     = 4, // enable OpenGL debug context
  };

  enum MouseModeEnum {
    MOUSE_NORMAL, // mouse cursor visible and behaves normally
    MOUSE_HIDE,   // hides mouse cursor when it is over display window
    MOUSE_DISABLE // hides & grabs mouse cursor and all movement events are
                  //   relative position changes
  };

  class Window;
  class Image;
  class Renderer;
  class Gui;
  class Texture;
}

class gx::Window
{
 public:
  Window() = default;
  ~Window();

  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;

  Window(Window&&) noexcept = default;
  Window& operator=(Window&&) noexcept = default;

  //// Display Management ////
  void setTitle(std::string_view title);
  void setSize(int width, int height, bool fullScreen);
  void setMouseMode(MouseModeEnum mode);
  void setMousePos(float x, float y);
  bool open(int flags = WINDOW_RESIZABLE | WINDOW_DECORATED);

  [[nodiscard]] int width() const { return _width; }
  [[nodiscard]] int height() const { return _height; }
  [[nodiscard]] const std::string& title() const { return _title; }
  [[nodiscard]] bool fullScreen() const { return _fullScreen; }
  [[nodiscard]] int maxTextureSize() const { return _maxTextureSize; }

  //// Render Functions ////
  void setBGColor(float r, float g, float b);
  void setBGColor(const Color& c) { setBGColor(c.r, c.g, c.b); }
  void clearFrame();
  void draw(const DrawList& dl);
  void draw(const DrawList& dl, const Color& modColor);

  template <typename Drawable>
  void draw(const Drawable& d) { draw(d.drawList()); }

  template <typename Drawable, typename... Args>
  void draw(const Drawable& d, const Args&... args) {
    draw(d.drawList(), args...); }

  void renderFrame();


  //// Event Handling ////
  int pollEvents();
    // updates event state, returns current event mask

  [[nodiscard]] int64_t lastPollTime() const { return _lastPollTime; }
    // time of last pollEvents()
    // (in microseconds since first window open)

  [[nodiscard]] int events() const { return _events; }
  [[nodiscard]] int removedEvents() const { return _removedEvents; }
    // current event mask

  void removeEvent(int event_mask) {
    int e = _events & event_mask; _removedEvents |= e; _events &= ~e; }
    // remove event(s) from current event mask

  // window state
  [[nodiscard]] bool resized() const { return _events & EVENT_SIZE; }
  [[nodiscard]] bool closed() const { return _events & EVENT_CLOSE; }

  // mouse state
  [[nodiscard]] double mouseX() const { return _mouseX; }
  [[nodiscard]] double mouseY() const { return _mouseY; }
  [[nodiscard]] double scrollX() const { return _scrollX; }
  [[nodiscard]] double scrollY() const { return _scrollY; }
  [[nodiscard]] int buttons() const { return _buttons; }
  [[nodiscard]] int mods() const { return _mods; }
  [[nodiscard]] bool mouseIn() const { return _mouseIn; }
  [[nodiscard]] bool iconified() const { return _iconified; }
  [[nodiscard]] bool focused() const { return _focused; }

  // key state
  [[nodiscard]] bool keyPressed(int key) const;
  [[nodiscard]] int keyPressCount(int key, bool includeRepeat) const;
  [[nodiscard]] const std::vector<KeyState>& keyStates() const {
    return _keyStates; }

  // text entry state
  [[nodiscard]] const std::vector<CharInfo>& charData() const {
    return _chars; }

 protected:
  friend class Texture;
  std::shared_ptr<Renderer> _renderer;

 private:
  int _width = 0, _height = 0;
  int _fsWidth = 0, _fsHeight = 0;
  int _minWidth = -1, _minHeight = -1;
  int _maxWidth = -1, _maxHeight = -1;
  std::string _title;
  MouseModeEnum _mouseMode = MOUSE_NORMAL;
  bool _sizeSet = false;
  bool _fullScreen = false;
  int _maxTextureSize = 0;

  // event state
  int64_t _lastPollTime = 0;
  int _events = 0, _removedEvents = 0;
  std::vector<KeyState> _keyStates;
  std::vector<CharInfo> _chars;
  float _mouseX = 0, _mouseY = 0;
  float _scrollX = 0, _scrollY = 0;
  int _buttons = 0, _mods = 0;
  bool _mouseIn = false, _iconified = false, _focused = true;

  void showWindow(GLFWwindow*);
  void updateMouseState(GLFWwindow*);

  // GLFW event callbacks
  static void closeCB(GLFWwindow*);
  static void sizeCB(GLFWwindow*, int width, int height);
  static void keyCB(GLFWwindow*, int key, int scancode, int action, int mods);
  static void charCB(GLFWwindow*, unsigned int codepoint);
  static void cursorEnterCB(GLFWwindow*, int entered);
  static void cursorPosCB(GLFWwindow*, double xpos, double ypos);
  static void mouseButtonCB(GLFWwindow*, int button, int action, int mods);
  static void scrollCB(GLFWwindow*, double xoffset, double yoffset);
  static void iconifyCB(GLFWwindow*, int iconified);
  static void focusCB(GLFWwindow*, int focused);
};
