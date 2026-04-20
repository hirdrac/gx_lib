//
// gx/EventState.hh
// Copyright (C) 2026 Richard Bradley
//
// state of Window events & user input
//

#pragma once
#include "Types.hh"
#include <vector>
#include <string>
#include <algorithm>


namespace gx {
  // Event Types
  enum EventEnum {
    // window events
    EVENT_CLOSE        = 1<<0,
    EVENT_SIZE         = 1<<1,
    EVENT_ICONIFY      = 1<<2,
    EVENT_FOCUS        = 1<<3,

    // keyboard events
    EVENT_KEY          = 1<<4,
    EVENT_TEXT         = 1<<5,

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

  enum KeyEnum : int16_t {
    // key values
    KEY_UNKNOWN = -1,

    // printable keys
    KEY_SPACE = ' ', KEY_APOSTROPHE = '\'', KEY_COMMA = ',',
    KEY_MINUS = '-', KEY_PERIOD = '.', KEY_SLASH = '/',
    KEY_0 = '0', KEY_1 = '1', KEY_2 = '2', KEY_3 = '3', KEY_4 = '4',
    KEY_5 = '5', KEY_6 = '6', KEY_7 = '7', KEY_8 = '8', KEY_9 = '9',
    KEY_SEMICOLON = ';', KEY_EQUAL = '=',
    KEY_A = 'A', KEY_B = 'B', KEY_C = 'C', KEY_D = 'D', KEY_E = 'E',
    KEY_F = 'F', KEY_G = 'G', KEY_H = 'H', KEY_I = 'I', KEY_J = 'J',
    KEY_K = 'K', KEY_L = 'L', KEY_M = 'M',
    KEY_N = 'N', KEY_O = 'O', KEY_P = 'P', KEY_Q = 'Q', KEY_R = 'R',
    KEY_S = 'S', KEY_T = 'T', KEY_U = 'U', KEY_V = 'U', KEY_W = 'W',
    KEY_X = 'X', KEY_Y = 'Y', KEY_Z = 'Z',
    KEY_LBRACKET = '[', KEY_BACKSLASH = '\\', KEY_RBRACKET = ']',
    KEY_BACKTICK = '`',

    // function keys
    KEY_ESCAPE = '\x1B', KEY_ENTER = '\r',
    KEY_TAB = '\t', KEY_BACKSPACE = '\b',
    KEY_INSERT = 260, KEY_DELETE = 261,
    KEY_RIGHT = 262, KEY_LEFT = 263, KEY_DOWN = 264, KEY_UP = 265,
    KEY_PAGE_UP = 266, KEY_PAGE_DOWN = 267, KEY_HOME = 268, KEY_END = 269,
    KEY_CAPS_LOCK = 280, KEY_SCROLL_LOCK = 281, KEY_NUM_LOCK = 282,
    KEY_PRINT_SCREEN = 283, KEY_PAUSE = 284,
    KEY_F1 = 290, KEY_F2 = 291, KEY_F3 = 292, KEY_F4 = 293, KEY_F5 = 294,
    KEY_F6 = 295, KEY_F7 = 296, KEY_F8 = 297, KEY_F9 = 298, KEY_F10 = 299,
    KEY_F11 = 300, KEY_F12 = 301, KEY_F13 = 302, KEY_F14 = 303, KEY_F15 = 304,
    KEY_F16 = 305, KEY_F17 = 306, KEY_F18 = 307, KEY_F19 = 308, KEY_F20 = 309,
    KEY_F21 = 310, KEY_F22 = 311, KEY_F23 = 312, KEY_F24 = 313, KEY_F25 = 314,
    KEY_LSHIFT = 340, KEY_LCONTROL = 341,
    KEY_LALT = 342, KEY_LSUPER = 343,
    KEY_RSHIFT = 344, KEY_RCONTROL = 345,
    KEY_RALT = 346, KEY_RSUPER = 347, KEY_MENU = 348,

    // keypad keys
    KEY_KP_0 = 320, KEY_KP_1 = 321, KEY_KP_2 = 322, KEY_KP_3 = 323,
    KEY_KP_4 = 324, KEY_KP_5 = 325, KEY_KP_6 = 326, KEY_KP_7 = 327,
    KEY_KP_8 = 328, KEY_KP_9 = 329,
    KEY_KP_DECIMAL = 330, KEY_KP_DIVIDE = 331, KEY_KP_MULTIPLY = 332,
    KEY_KP_SUBTRACT = 333, KEY_KP_ADD = 334,
    KEY_KP_ENTER = 335, KEY_KP_EQUAL = 336,
  };

  struct KeyState {
    KeyEnum key;         // InputEnum value
    int16_t scancode;    // platform-specific key value
    int8_t pressCount;   // number of press events since pollEvents
    int8_t repeatCount;  // number of repeat events since pollEvents
    int8_t held;         // key is currently held
  };

  struct ButtonState {
    int16_t button;      // 1 left button, 2 right button, 3 middle button
    int8_t  pressCount;  // number of press events since pollEvents
    int8_t  held;        // button is currently held
  };

  struct EventState;
}

struct gx::EventState
{
  int64_t lastPollTime;
  std::vector<KeyState> keyStates;
  std::vector<ButtonState> buttonStates;
  std::string text;  // UTF8 encoded
  Vec2 mousePt, scrollPt;
  int events, mods, shiftCount, controlCount, altCount, superCount;
  bool mouseIn, iconified, focused;

  // helper functions
  [[nodiscard]] bool closed() const { return events & EVENT_CLOSE; }
  [[nodiscard]] bool resized() const { return events & EVENT_SIZE; }
  [[nodiscard]] bool iconfifyEvent() const { return events & EVENT_ICONIFY; }
  [[nodiscard]] bool focusEvent() const { return events & EVENT_FOCUS; }
  [[nodiscard]] bool keyEvent() const { return events & EVENT_KEY; }
  [[nodiscard]] bool textEvent() const { return events & EVENT_TEXT; }
  [[nodiscard]] bool buttonEvent() const { return events & EVENT_MOUSE_BUTTON; }
  [[nodiscard]] bool mouseInEvent() const { return events & EVENT_MOUSE_ENTER; }
  [[nodiscard]] bool mouseMove() const { return events & EVENT_MOUSE_MOVE; }
  [[nodiscard]] bool mouseScroll() const { return events & EVENT_MOUSE_SCROLL; }

  // key state
  [[nodiscard]] const KeyState* getKeyState(KeyEnum key) const {
    for (const KeyState& ks : keyStates) {
      if (ks.key == key) { return &ks; }
    }
    return nullptr;
  }

  [[nodiscard]] bool keyHeld(KeyEnum key) const {
    const KeyState* ks = getKeyState(key);
    return ks && (ks->held || (ks->pressCount > 0));
  }

  [[nodiscard]] int keyPressCount(
    KeyEnum key, bool includeRepeat = false) const {
    const KeyState* ks = getKeyState(key);
    return ks ? (ks->pressCount + (int(includeRepeat) * ks->repeatCount)) : 0;
  }

  [[nodiscard]] bool keyPress(
    KeyEnum key, bool includeRepeat = false) const {
    return keyPressCount(key, includeRepeat) > 0;
  }

  [[nodiscard]] bool keyRelease(KeyEnum key) const {
    const KeyState* ks = getKeyState(key);
    return ks && !ks->held;
  }

  // button state
  [[nodiscard]] const ButtonState* getButtonState(int16_t button) const {
    for (const ButtonState& bs : buttonStates) {
      if (bs.button == button) { return &bs; }
    }
    return nullptr;
  }

  [[nodiscard]] bool buttonHeld(int16_t button) const {
    const ButtonState* bs = getButtonState(button);
    return bs && (bs->held || (bs->pressCount > 0));
  }

  [[nodiscard]] int buttonPressCount(int16_t button) const {
    const ButtonState* bs = getButtonState(button);
    return bs ? bs->pressCount : 0;
  }

  [[nodiscard]] bool buttonPress(int16_t button) const {
    return buttonPressCount(button) > 0;
  }

  [[nodiscard]] bool buttonRelease(int16_t button) const {
    const ButtonState* bs = getButtonState(button);
    return bs && !bs->held;
  }

  [[nodiscard]] bool buttonDrag(int16_t button) const {
    return mouseMove() && buttonHeld(button);
  }

  // remove/consume events
  void removeKeyEvent(KeyEnum key) {
    auto itr = std::find_if(keyStates.begin(), keyStates.end(),
                            [key](auto& ks){ return ks.key == key; });
    if (itr != keyStates.end()) {
      keyStates.erase(itr);
      if (keyStates.empty()) { removeEvent(EVENT_KEY); }
    }
  }

  void removeKeyEvent() {
    removeEvent(EVENT_KEY);
    keyStates.clear();
  }

  void removeButtonEvent(int16_t button) {
    auto itr = std::find_if(buttonStates.begin(), buttonStates.end(),
                            [button](auto& bs){ return bs.button == button; });
    if (itr != buttonStates.end()) {
      buttonStates.erase(itr);
      if (buttonStates.empty()) { removeEvent(EVENT_MOUSE_BUTTON); }
    }
  }

  void removeButtonEvent() {
    removeEvent(EVENT_MOUSE_BUTTON);
    buttonStates.clear();
  }

  void removeTextEvent() {
    removeEvent(EVENT_TEXT);
    text.clear();
  }

  void removeEvent(int eventMask) {
    events &= ~(events & eventMask);
  }

  void reset() {
    events = 0;
    scrollPt.set(0,0);
    text.clear();

    for (auto i = keyStates.begin(); i != keyStates.end(); ) {
      if (i->held) {
        i->pressCount = 0;
        i->repeatCount = 0;
        ++i;
      } else {
        i = keyStates.erase(i);
      }
    }

    for (auto i = buttonStates.begin(); i != buttonStates.end(); ) {
      if (i->held) {
        i->pressCount = 0;
        ++i;
      } else {
        i = buttonStates.erase(i);
      }
    }
  }
};
