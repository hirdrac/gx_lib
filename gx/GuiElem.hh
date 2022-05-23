//
// gx/GuiElem.hh
// Copyright (C) 2022 Richard Bradley
//

#pragma once
#include "Align.hh"
#include "Types.hh"
#include <vector>
#include <string>
#include <string_view>
#include <initializer_list>
#include <utility>


namespace gx {
  using ElemID = int32_t;   // internal GuiElem ID
  using EventID = int32_t;  // user specified event value

  class GuiTexture;
  struct GuiElem;

  enum GuiElemType {
    GUI_NULL = 0,

    // layout types
    GUI_HFRAME, GUI_VFRAME, GUI_SPACER,

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

  enum GuiAction {
    ACTION_NONE = 0,
  };
}


class gx::GuiTexture
{
 public:
  GuiTexture() = default;
  GuiTexture(TextureID id) : _tid{id} { }
  ~GuiTexture() { cleanup(); }

  // copy/assign (new copy does not receive texture)
  GuiTexture(const GuiTexture&) { }
  GuiTexture& operator=(const GuiTexture&) {
    cleanup(); _tid = 0; return *this; }

  // move/move-assign (transfer texture ownership)
  GuiTexture(GuiTexture&& t) : _tid{std::exchange(t._tid, 0)} { }
  GuiTexture& operator=(GuiTexture&& t) {
    cleanup(); _tid = std::exchange(t._tid, 0); return *this; }

  // other methods
  [[nodiscard]] explicit operator bool() const { return _tid != 0; }
  [[nodiscard]] TextureID id() const { return _tid; }

 private:
  TextureID _tid = 0;
  void cleanup();
};


struct gx::GuiElem
{
  // shared properties
  std::vector<GuiElem> elems;  // child elements
  std::string text;  // label/entry text
  GuiElemType type;
  AlignEnum align;
  EventID eid;
  GuiTexture tex;    // texture cache for rendering

  // elem type specific properties
  struct LabelProps {
    float minLength;
  };

  struct SpacerProps {
    int16_t left, top, right, bottom;
  };

  struct ButtonProps {
    int64_t repeatDelay; // BUTTON_PRESS only
    GuiAction action;
    EventID targetID;
  };

  struct EntryProps {
    float size; // width in characters
    uint32_t maxLength;
    EntryType type;
    AlignEnum align; // alignment of entry text
  };

  struct ImageProps {
    float width, height;
    TextureID texId;
    Vec2 texCoord0, texCoord1;
  };

  union {
    bool checkboxSet;    // CHECKBOX
    int itemNo;          // LISTSELECT, LISTSELECT_ITEM
    LabelProps label;    // LABEL,VLABEL
    SpacerProps spacer;  // SPACER
    ButtonProps button;  // BUTTON,BUTTON_PRESS
    EntryProps entry;    // ENTRY
    ImageProps image;    // IMAGE
  };

  // layout state
  ElemID _id = 0;
  float _x = 0, _y = 0;  // layout position relative to panel
  float _w = 0, _h = 0;  // layout size
  bool _active = false;  // popup/menu activated
  bool _enabled = true;

  GuiElem()
    : type{GUI_NULL}, align{ALIGN_UNSPECIFIED}, eid{0} { }
  GuiElem(GuiElemType t, AlignEnum a, EventID i)
    : type{t}, align{a}, eid{i} { }
  GuiElem(GuiElemType t, AlignEnum a, EventID i, std::string_view txt)
    : text{txt}, type{t}, align{a}, eid{i} { }
  GuiElem(GuiElemType t, AlignEnum a, EventID i,
          std::initializer_list<GuiElem> x)
    : elems{x}, type{t}, align{a}, eid{i} { }
};
