//
// gx/Window.hh
// Copyright (C) 2024 Richard Bradley
//
// encapsulation of OS specific window handling & user input
//

#pragma once
#include "Renderer.hh"
#include "Types.hh"
#include <string_view>
#include <string>
#include <vector>
#include <memory>


namespace gx {
  // Window Setting Values
  enum WindowFlagEnum {
    WINDOW_DECORATED = 1,
      // use decorations when not fullscreen

    // resizing flags
    WINDOW_RESIZABLE = 2,
      // can be resized when not fullscreen (decorated implied)
    WINDOW_FIXED_ASPECT_RATIO = 4,
      // set resize aspect ratio based on initial size
    WINDOW_LIMIT_MIN_SIZE = 8,
      // use initial size as min resize limit
    WINDOW_LIMIT_MAX_SIZE = 16,
      // use initial size as max resize limit

    // context flags
    WINDOW_DEBUG = 32,
      // enable OpenGL debug context
  };

  enum MouseModeEnum {
    MOUSEMODE_NORMAL,  // mouse cursor visible and behaves normally
    MOUSEMODE_HIDE,    // hides mouse cursor when it is over display window
    MOUSEMODE_DISABLE, // hides & grabs mouse cursor and all movement events
                       //   are relative position changes
  };

  enum MouseShapeEnum {
    MOUSESHAPE_ARROW,
    MOUSESHAPE_IBEAM,
    MOUSESHAPE_CROSSHAIR,
  };


  // Event Types
  enum EventEnum {
    // window events
    EVENT_CLOSE        = 1<<0,
    EVENT_SIZE         = 1<<1,
    EVENT_ICONIFY      = 1<<2,
    EVENT_FOCUS        = 1<<3,

    // key/char events
    EVENT_KEY          = 1<<4,
    EVENT_CHAR         = 1<<5,

    // mouse events
    EVENT_MOUSE_BUTTON = 1<<6,
    EVENT_MOUSE_ENTER  = 1<<7,
    EVENT_MOUSE_MOVE   = 1<<8,
    EVENT_MOUSE_SCROLL = 1<<9,

    // groups
    EVENT_INPUT = EVENT_KEY | EVENT_MOUSE_BUTTON,
  };

  enum ModEnum {
    MOD_SHIFT = 1<<0, MOD_CONTROL = 1<<1, MOD_ALT = 1<<2, MOD_SUPER = 1<<3,
  };

  enum InputEnum {
    // key values (adapted from glfw3.h)
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
    KEY_RALT = 346, KEY_RSUPER = 347, KEY_MENU = 348,

    // mouse buttons
    BUTTON_1 = 1001,  // left mouse button
    BUTTON_2 = 1002,  // right mouse button
    BUTTON_3 = 1003,  // middle mouse button
    BUTTON_4 = 1004,
    BUTTON_5 = 1005,
    BUTTON_6 = 1006,
    BUTTON_7 = 1007,
    BUTTON_8 = 1008,
  };

  struct InputState {
    int16_t value;       // InputEnum value
    int16_t scancode;    // platform-specific key value
    uint8_t mods;        // mods value at press
    int8_t pressCount;   // number of press events since pollEvents
    int8_t repeatCount;  // number of repeat events since pollEvents
    int8_t held;         // key is currently held
  };
  static_assert(sizeof(InputState) == 8);

  struct EventState {
    int events, removedEvents;
    std::vector<InputState> inputStates;
    std::vector<int32_t> chars;
    Vec2 mousePt, scrollPt;
    int mods, shiftCount, controlCount, altCount, superCount;
    bool mouseIn, iconified, focused;

    // helper functions
    [[nodiscard]] int allEvents() const { return events | removedEvents; }
    [[nodiscard]] bool resized() const { return events & EVENT_SIZE; }
    [[nodiscard]] bool closed() const { return events & EVENT_CLOSE; }
    [[nodiscard]] bool mouseMove() const { return events & EVENT_MOUSE_MOVE; }
    [[nodiscard]] bool mouseScroll() const {
      return events & EVENT_MOUSE_SCROLL; }

    [[nodiscard]] const InputState* getInputState(InputEnum value) const {
      for (const InputState& in : inputStates) {
        if (in.value == value) { return &in; }
      }
      return nullptr;
    }

    [[nodiscard]] bool inputHeld(InputEnum value) const {
      const InputState* in = getInputState(value);
      return in && (in->held || (in->pressCount > 0));
    }

    [[nodiscard]] int inputPressCount(
      InputEnum value, bool includeRepeat = false) const {
      const InputState* in = getInputState(value);
      if (in && (in->value == value)) {
        return in->pressCount + (int(includeRepeat) * in->repeatCount);
      } else {
        return 0;
      }
    }

    [[nodiscard]] bool inputPress(
      InputEnum value, bool includeRepeat = false) const {
      return inputPressCount(value, includeRepeat) > 0;
    }

    [[nodiscard]] bool inputRelease(InputEnum value) const {
      const InputState* in = getInputState(value);
      return in && !in->held;
    }

    [[nodiscard]] bool inputDrag(InputEnum value) const {
      return mouseMove() && inputHeld(value); }
  };


  class Window;
  class Renderer;
  struct WindowImpl;
}

class gx::Window
{
 public:
  Window();
  ~Window();

  // prevent copy/assignment
  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;

  // allow move/move-assign
  Window(Window&&) = default;
  Window& operator=(Window&&) = default;


  //// Display Management ////
  void setTitle(std::string_view title);
  void setSize(int width, int height, bool fullScreen);
  void setSizeLimits(int minWidth, int minHeight, int maxWidth, int maxHeight);
  void setMouseMode(MouseModeEnum mode);
  void setMouseShape(MouseShapeEnum shape);
  void setMousePos(Vec2 pos);
  void setSamples(int samples);

  bool open(int flags = WINDOW_RESIZABLE);
  [[nodiscard]] bool isOpen() const;
  [[nodiscard]] explicit operator bool() const { return isOpen(); }

  [[nodiscard]] int width() const;
  [[nodiscard]] int height() const;
  [[nodiscard]] std::pair<int,int> dimensions() const;
  [[nodiscard]] const std::string& title() const;
  [[nodiscard]] bool fullScreen() const;
  [[nodiscard]] MouseModeEnum mouseMode() const;
  [[nodiscard]] MouseShapeEnum mouseShape() const;


  //// Event Handling ////
  static int pollEvents();
    // updates event state for all windows, returns combined event mask
    // (each window should be checked for events if returned value is non-zero)

  [[nodiscard]] static int64_t lastPollTime() { return _lastPollTime; }
    // time of last pollEvents()
    // (in microseconds since first window open)

  [[nodiscard]] const EventState& eventState() const;
    // current event state for window

  void removeEvent(int event_mask);
    // remove event(s) from current event mask

  // renderer access methods
  [[nodiscard]] Renderer& renderer();
    // NOTE: Renderer is available once open() is called and will be available
    //   until the Window is destroyed.

  template<class... Args>
  void draw(Args&&... args) { renderer().draw({&args...}); }

  void renderFrame() { renderer().renderFrame(_lastPollTime); }

 private:
  std::unique_ptr<WindowImpl> _impl;
  static int64_t _lastPollTime;
};
