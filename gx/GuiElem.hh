//
// gx/GuiElem.hh
// Copyright (C) 2026 Richard Bradley
//

#pragma once
#include "Renderer.hh"
#include "Align.hh"
#include "Types.hh"
#include <vector>
#include <string>
#include <initializer_list>
#include <variant>


namespace gx {
  using ElemID = int32_t;   // internal GuiElem ID
  using EventID = int32_t;  // user specified event value

  class GuiElem;

  enum GuiElemType {
    GUI_NULL = 0,

    // layout types
    GUI_HFRAME, GUI_VFRAME,

    // draw types
    GUI_PANEL, GUI_POPUP,
    GUI_LABEL, GUI_VLABEL,
    GUI_HLINE, GUI_VLINE, GUI_IMAGE,
    GUI_TITLEBAR,

    // event types
    GUI_BUTTON,          // activated on release
    GUI_BUTTON_PRESS,    // activated on press, optionally repeat if held
    GUI_CHECKBOX,        // toggle value on release
    GUI_MENU,            // button hold or release on menu opens menu
    GUI_MENU_ITEM,       // activated on press or release
    GUI_SUBMENU,         // as GUI_MENU
    GUI_LISTSELECT,      // as GUI_MENU
    GUI_LISTSELECT_ITEM, // as GUI_MENU_ITEM
    GUI_ENTRY,           // activated if changed on enter/tab/click-away
  };

  enum EntryType {
    ENTRY_TEXT,     // all characters valid
    ENTRY_CARDINAL, // positive integer
    ENTRY_INTEGER,  // positive/negative integer
    ENTRY_FLOAT,    // floating point number
    ENTRY_PASSWORD  // all characters valid w/ output hidden
  };

  struct GuiAction {
    enum {
      NONE = 0,
      ADD = 1,    // add value to target
      SET = 2     // set target to value
    } type = NONE;
    EventID targetID = 0;
    double value = 0;
  };
}


class gx::GuiElem
{
 public:
  // shared properties
  std::vector<GuiElem> elems;  // child elements
  GuiElemType type;
  Align align;
  EventID eid;

  // layout margins
  int16_t l_margin = 0; // left
  int16_t t_margin = 0; // top
  int16_t r_margin = 0; // right
  int16_t b_margin = 0; // bottom

  // elem type specific properties
  struct LabelProps {
    std::string text;
    float minLength;
    int32_t minLines;
  };

  struct ButtonProps {
    int64_t repeatDelay = -1; // BUTTON_PRESS only (-1 disables)
    GuiAction action{};
  };

  struct CheckboxProps {
    bool set = false;
  };

  struct ItemProps {
    int no = 0;
  };

  struct EntryProps {
    std::string text;
    float size; // width in characters
    uint32_t maxChars;
    EntryType type;
    Align align; // alignment of entry text
    float tx = 0; // cache last text x pos for mouse click cursor pos calc
  };

  struct ImageProps {
    float width, height;
    TextureID texId;
    Vec2 texCoord0{INIT_NONE}, texCoord1{INIT_NONE};
  };

  std::variant<
    LabelProps,    // LABEL,VLABEL
    ButtonProps,   // BUTTON,BUTTON_PRESS
    CheckboxProps, // CHECKBOX
    ItemProps,     // LISTSELECT,LISTSELECT_ITEM,MENU_ITEM
    EntryProps,    // ENTRY
    ImageProps     // IMAGE
    > props;

#define GX_GETTER(name,type)\
  [[nodiscard]] type& name() { return std::get<type>(props); }\
  [[nodiscard]] const type& name() const { return std::get<type>(props); }

  GX_GETTER(label,LabelProps)
  GX_GETTER(button,ButtonProps)
  GX_GETTER(checkbox,CheckboxProps)
  GX_GETTER(item,ItemProps)
  GX_GETTER(entry,EntryProps)
  GX_GETTER(image,ImageProps)
#undef GX_GETTER

  // layout state
  ElemID _id = 0;
  float _x = 0, _y = 0;  // element position relative to panel
  float _w = 0, _h = 0;  // element size
    // NOTE: position/size doesn't include margins
  bool _active = false;  // popup/menu activated
  bool _enabled = true;

  GuiElem(GuiElemType t, Align a, EventID i)
    : type{t}, align{a}, eid{i} { }
  GuiElem(GuiElemType t, Align a, EventID i, std::initializer_list<GuiElem> x)
    : elems{x}, type{t}, align{a}, eid{i} { }

  [[nodiscard]] bool contains(float x, float y) const {
    return (x >= _x) && (x < (_x + _w)) && (y >= _y) && (y < (_y + _h));
  }

  [[nodiscard]] float marginW() const { return l_margin + r_margin; }
  [[nodiscard]] float marginH() const { return t_margin + b_margin; }

  [[nodiscard]] float layoutW() const { return _w + marginW(); }
  [[nodiscard]] float layoutH() const { return _h + marginH(); }
};
