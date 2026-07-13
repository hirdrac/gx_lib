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

    // keyboard/mouse events
    EVENT_INPUT        = 1<<4,
    EVENT_TEXT         = 1<<5,
    EVENT_MOUSE_ENTER  = 1<<6,
    EVENT_MOUSE_MOVE   = 1<<7,
    EVENT_MOUSE_SCROLL = 1<<8,
  };

  enum ModifierEnum {
    MODIFIER_SHIFT = 1<<0,
    MODIFIER_CTRL  = 1<<1,
    MODIFIER_ALT   = 1<<2,
    MODIFIER_SUPER = 1<<3,
  };

  enum InputEnum : int16_t {
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

    // mouse buttons
    BUTTON_1 = 1001,  // LMB
    BUTTON_2 = 1002,  // RMB
    BUTTON_3 = 1003,  // MMB
    BUTTON_4 = 1004,
    BUTTON_5 = 1005,
    BUTTON_6 = 1006,
    BUTTON_7 = 1007,
    BUTTON_8 = 1008,
  };

  struct InputState {
    InputEnum val;        // InputEnum value
    int16_t scancode;     // platform-specific input key value
    uint8_t pressCount;   // number of press events since pollEvents
    uint8_t repeatCount;  // number of repeat events since pollEvents
    bool held;            // input is currently held

    [[nodiscard]] bool event() const {
      return (pressCount > 0) || (repeatCount > 0) || !held;
    }
  };
  static_assert(sizeof(InputState) == 8);
}

struct gx::EventState
{
  int64_t lastPollTime;
  std::vector<InputState> inputStates;
  std::string text;  // UTF8 encoded
  Vec2 mousePt, scrollPt;
  int events, mods;
  bool mouseIn, iconified, focused;

  // event type helper functions
  [[nodiscard]] bool closed() const { return events & EVENT_CLOSE; }
  [[nodiscard]] bool resized() const { return events & EVENT_SIZE; }
  [[nodiscard]] bool iconfifyEvent() const { return events & EVENT_ICONIFY; }
  [[nodiscard]] bool focusEvent() const { return events & EVENT_FOCUS; }
  [[nodiscard]] bool inputEvent() const { return events & EVENT_INPUT; }
  [[nodiscard]] bool textEvent() const { return events & EVENT_TEXT; }
  [[nodiscard]] bool mouseInEvent() const { return events & EVENT_MOUSE_ENTER; }
  [[nodiscard]] bool mouseMove() const { return events & EVENT_MOUSE_MOVE; }
  [[nodiscard]] bool mouseScroll() const { return events & EVENT_MOUSE_SCROLL; }

  // input state
  [[nodiscard]] const InputState* getInputState(InputEnum val) const {
    for (const InputState& s : inputStates) { if (s.val == val) return &s; }
    return nullptr;
  }

  [[nodiscard]] InputState* getInputState(InputEnum val) {
    for (InputState& s : inputStates) { if (s.val == val) return &s; }
    return nullptr;
  }

  [[nodiscard]] bool inputEvent(InputEnum val) const {
    const InputState* s = getInputState(val);
    return s && s->event();
  }

  [[nodiscard]] bool inputHeld(InputEnum val) const {
    const InputState* s = getInputState(val);
    return s && (s->held || (s->pressCount > 0));
  }

  [[nodiscard]] int inputPressCount(
    InputEnum val, bool includeRepeat = false) const {
    const InputState* s = getInputState(val);
    return s ? (s->pressCount + (int(includeRepeat) * s->repeatCount)) : 0;
  }

  [[nodiscard]] bool inputPress(
    InputEnum val, bool includeRepeat = false) const {
    return inputPressCount(val, includeRepeat) > 0;
  }

  [[nodiscard]] bool inputRelease(InputEnum val) const {
    const InputState* s = getInputState(val);
    return s && !s->held;
  }

  [[nodiscard]] bool inputDrag(InputEnum val) const {
    return mouseMove() && inputHeld(val);
  }

  void addInputPress(InputEnum val, int scancode = 0) {
    InputState* s = getInputState(val);
    if (s) {
      ++s->pressCount;
      s->held = true;
    } else {
      inputStates.insert(inputStates.end(), {val,int16_t(scancode),1,0,true});
    }
    updateMod();
  }

  void addInputRepeat(InputEnum val) {
    InputState* s = getInputState(val);
    if (s) { ++s->repeatCount; }
  }

  void addInputRelease(InputEnum val) {
    InputState* s = getInputState(val);
    if (s) { s->held = false; updateMod(); }
  }

  // remove/consume events
  void removeInputEvent(InputEnum val) {
    auto itr = std::find_if(inputStates.begin(), inputStates.end(),
                            [val](auto& s){ return s.val == val; });
    if (itr != inputStates.end()) {
      inputStates.erase(itr);
      if (inputStates.empty()) { removeEvent(EVENT_INPUT); }
    }
  }

  void removeInputEvents() {
    removeEvent(EVENT_INPUT);
    inputStates.clear();
    mods = 0;
  }

  void removeButtonEvents() {
    for (auto i = inputStates.begin(); i != inputStates.end(); ) {
      if (i->val >= BUTTON_1) { i = inputStates.erase(i); } else { ++i; }
    }
    if (inputStates.empty()) { removeEvent(EVENT_INPUT); }
  }

  void removeKeyEvents() {
    for (auto i = inputStates.begin(); i != inputStates.end(); ) {
      if (i->val < BUTTON_1) { i = inputStates.erase(i); } else { ++i; }
    }
    if (inputStates.empty()) { removeEvent(EVENT_INPUT); }
    mods = 0;
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

    for (auto i = inputStates.begin(); i != inputStates.end(); ) {
      if (i->held) {
        i->pressCount = i->repeatCount = 0;
        ++i;
      } else {
        i = inputStates.erase(i);
      }
    }

    updateMod();
  }

  void updateMod() {
    mods = 0;
    for (InputState& s : inputStates) {
      if (!s.held) { continue; }
      switch (s.val) {
        default: break;
        case KEY_LSHIFT:   case KEY_RSHIFT:   mods |= MODIFIER_SHIFT; break;
        case KEY_LCONTROL: case KEY_RCONTROL: mods |= MODIFIER_CTRL; break;
        case KEY_LALT:     case KEY_RALT:     mods |= MODIFIER_ALT; break;
        case KEY_LSUPER:   case KEY_RSUPER:   mods |= MODIFIER_SUPER; break;
      }
    }
  }
};
