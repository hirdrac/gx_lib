//
// gx/EventState.hh
// Copyright (C) 2025 Richard Bradley
//
// state of Window events & user input
//

#pragma once
#include "Types.hh"
#include <vector>


namespace gx {
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
  };

  enum ModifierEnum {
    MODIFIER_SHIFT = 1<<0,
    MODIFIER_CTRL  = 1<<1,
    MODIFIER_ALT   = 1<<2,
    MODIFIER_SUPER = 1<<3,
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

  constexpr bool isButtonInput(InputEnum val) {
    return (val >= BUTTON_1) && (val <= BUTTON_8);
  }

  constexpr bool isKeyInput(InputEnum val) {
    return !isButtonInput(val);
  }

  struct InputState {
    int16_t value;       // InputEnum value
    int16_t scancode;    // platform-specific key value
    int8_t pressCount;   // number of press events since pollEvents
    int8_t repeatCount;  // number of repeat events since pollEvents
    int8_t held;         // key is currently held
  };
  static_assert(sizeof(InputState) == 8);

  struct EventState;
}

struct gx::EventState
{
  int64_t lastPollTime;
  std::vector<InputState> keyStates, buttonStates;
  std::vector<int32_t> chars;
  Vec2 mousePt, scrollPt;
  int events, mods, shiftCount, controlCount, altCount, superCount;
  bool mouseIn, iconified, focused;

  // helper functions
  [[nodiscard]] bool resized() const { return events & EVENT_SIZE; }
  [[nodiscard]] bool closed() const { return events & EVENT_CLOSE; }
  [[nodiscard]] bool mouseMove() const { return events & EVENT_MOUSE_MOVE; }
  [[nodiscard]] bool mouseScroll() const {
    return events & EVENT_MOUSE_SCROLL; }
  [[nodiscard]] bool buttonEvent() const { return events & EVENT_MOUSE_BUTTON; }
  [[nodiscard]] bool keyEvent() const { return events & EVENT_KEY; }

  [[nodiscard]] const InputState* getInputState(InputEnum value) const {
    auto& states = isButtonInput(value) ? buttonStates : keyStates;
    for (const InputState& in : states) {
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

  // remove/consume events
  void removeInputEvent(InputEnum value) {
    const bool bv = isButtonInput(value);
    auto& states = bv ? buttonStates : keyStates;
    for (auto itr = states.begin(), end = states.end(); itr != end; ++itr) {
      if (itr->value == value) {
        states.erase(itr);
        if (states.empty()) {
          removeEvent(bv ? EVENT_MOUSE_BUTTON : EVENT_KEY);
        }
      }
    }
  }

  void removeKeyEvent() {
    removeEvent(EVENT_KEY);
    keyStates.clear();
  }

  void removeCharEvent() {
    removeEvent(EVENT_CHAR);
    chars.clear();
  }

  void removeMouseButtonEvent() {
    removeEvent(EVENT_MOUSE_BUTTON);
    buttonStates.clear();
  }

  void removeEvent(int eventMask) {
    events &= ~(events & eventMask);
  }
};
