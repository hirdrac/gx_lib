//
// gx/Gui.hh
// Copyright (C) 2022 Richard Bradley
//
// Graphical user interface rendering & event handling
//

#pragma once
#include "GuiTheme.hh"
#include "DrawList.hh"
#include "Align.hh"
#include "Types.hh"
#include <string>
#include <string_view>
#include <vector>
#include <initializer_list>
#include <memory>


namespace gx {
  using PanelID = int32_t;  // internal/external panel ID
  using ElemID = int32_t;   // internal GuiElem ID
  using EventID = int32_t;  // user specified event value

  class Window;
  class DrawContext;
  class Font;
  class Gui;
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
}


struct gx::GuiElem
{
  // shared properties
  std::vector<GuiElem> elems;  // child elements
  std::string text;  // label/entry text
  GuiElemType type;
  AlignEnum align;
  EventID eid;

  // elem type specific properties
  struct SpacerProps {
    int16_t left, top, right, bottom;
  };

  struct ButtonProps {
    int64_t repeatDelay; // BUTTON_PRESS only
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
    ButtonProps button;  // BUTTON,BUTTON_PRESS
    SpacerProps spacer;  // SPACER
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
  GuiElem(GuiElemType t, AlignEnum a, EventID i,
          std::initializer_list<GuiElem> x)
    : elems{x}, type{t}, align{a}, eid{i} { }
};


class gx::Gui
{
 public:
  inline PanelID newPanel(const GuiTheme& theme, float x, float y,
                          AlignEnum align, GuiElem&& elems);
  inline PanelID newPanel(const GuiTheme& theme, float x, float y,
                          AlignEnum align, const GuiElem& elems);
  void clear();
  void deletePanel(PanelID id);
  void raisePanel(PanelID id);
  void lowerPanel(PanelID id);

  bool getPanelLayout(PanelID id, Rect& layout) const;
  [[nodiscard]] PanelID topPanel() const {
    return _panels.empty() ? 0 : _panels.front()->id; }
  [[nodiscard]] PanelID bottomPanel() const {
    return _panels.empty() ? 0 : _panels.back()->id; }

  bool update(Window& win);
    // process events & update drawLists
    // returns true if redraw is required (same as needRedraw())

  [[nodiscard]] const DrawList& drawList() const { return _dl; }

  [[nodiscard]] EventID eventID() const { return _eventID; }
  [[nodiscard]] GuiElemType eventType() const { return _eventType; }
  [[nodiscard]] int64_t eventTime() const { return _eventTime; }
    // id/type of element triggering an event

  [[nodiscard]] bool needRedraw() const { return _needRedraw; }
    // true if GUI needs to be redrawn

  void setElemState(EventID eid, bool enable);
  void enableElem(EventID eid) { setElemState(eid, true); }
  void disableElem(EventID eid) { setElemState(eid, false); }
    // enable/disable event generating elements

  void setAllElemState(PanelID id, bool enable);
  void enableAllElem(PanelID id = 0) { setAllElemState(id, true); }
  void disableAllElem(PanelID id = 0) { setAllElemState(id, false); }
    // enable/disable all event generating elements
    // (optionally just for a specific panel if id is non-zero)

  [[nodiscard]] std::string getText(EventID eid) const {
    const GuiElem* e = findElemByEventID(eid);
    return (e == nullptr) ? std::string{} : e->text;
  }

  [[nodiscard]] bool getBool(EventID eid) const {
    const GuiElem* e = findElemByEventID(eid);
    return (e == nullptr || e->type != GUI_CHECKBOX) ? false : e->checkboxSet;
  }

  [[nodiscard]] int getItemNo(EventID eid) const {
    const GuiElem* e = findElemByEventID(eid);
    return (e == nullptr || e->type != GUI_LISTSELECT) ? 0 : e->itemNo;
  }

  [[nodiscard]] std::string eventText() const { return getText(_eventID); }
  [[nodiscard]] bool eventBool() const { return getBool(_eventID); }
  [[nodiscard]] int eventItemNo() const { return getItemNo(_eventID); }

  bool setText(EventID eid, std::string_view text);
  bool setBool(EventID eid, bool val);
  bool setItemNo(EventID eid, int item_no);

 private:
  struct Panel {
    const GuiTheme* theme;
    GuiElem root;
    PanelID id = 0;
    Rect layout{};
    bool needLayout = false;
  };

  // element definition
  using PanelPtr = std::unique_ptr<Panel>;
  std::vector<PanelPtr> _panels;
  PanelID _lastPanelID = 0;
  ElemID _lastElemID = 0;

  // current state
  DrawList _dl, _dl2;
  ElemID _hoverID = 0;
  ElemID _focusID = 0;

  ElemID _popupID = 0;
  GuiElemType _popupType = GUI_NULL;

  EventID _eventID = 0;
  GuiElemType _eventType = GUI_NULL;
  int64_t _eventTime = 0;

  ElemID _heldID = 0;
  GuiElemType _heldType = GUI_NULL;
  float _heldX = 0, _heldY = 0;
  int64_t _heldTime = 0;
  int64_t _repeatDelay = -1;  // negative value disables repeat
  std::size_t _cursorPos = 0; // character pos of cursor
  float _entryOffset = 0;     // text render offset because of cursor

  int64_t _lastCursorUpdate = 0;
  uint32_t _cursorBlinkTime = 0; // cached theme value
  bool _cursorState = false;  // track cursor blinking
  bool _needRender = true;
  bool _needRedraw = false;
  bool _textChanged = false;

  PanelID addPanel(PanelPtr ptr, float x, float y, AlignEnum align);
  void layout(Panel& p, float x, float y, AlignEnum align);
  void processMouseEvent(Window& win);
  void processCharEvent(Window& win);
  bool addEntryChar(GuiElem& e, int32_t codepoint);
  void setFocus(Window& win, const GuiElem* e);
  void initElem(GuiElem& def);
  void deactivatePopups();
  void activatePopup(const GuiElem& def);
  bool drawElem(
    Window& win, Panel& p, GuiElem& def, DrawContext& dc, DrawContext& dc2,
    const GuiTheme::Style* style);
  bool drawPopup(Window& win, Panel& p, GuiElem& def,
                 DrawContext& dc, DrawContext& dc2);

  [[nodiscard]] GuiElem* findElemByID(ElemID id);
  [[nodiscard]] GuiElem* findElemByEventID(EventID eid);
  [[nodiscard]] const GuiElem* findElemByEventID(EventID eid) const;
  [[nodiscard]] GuiElem* findNextElem(ElemID id, GuiElemType type = GUI_NULL);
  [[nodiscard]] GuiElem* findPrevElem(ElemID id, GuiElemType type = GUI_NULL);

  void clearHeld() {
    _heldID = 0;
    _heldType = GUI_NULL;
    _heldTime = 0;
    _repeatDelay = -1;
  }

  void setEvent(const GuiElem& e, int64_t t) {
    _eventID = e.eid;
    _eventType = e.type;
    _eventTime = t;
  }

  PanelPtr removePanel(PanelID id) {
    for (auto i = _panels.begin(), end = _panels.end(); i != end; ++i) {
      if ((*i)->id == id) {
        auto ptr = std::move(*i);
        _panels.erase(i);
        return ptr;
      }
    }
    return {};
  }
};

// **** Inline Implementations ****
gx::PanelID gx::Gui::newPanel(const GuiTheme& theme, float x, float y,
                              AlignEnum align, GuiElem&& elems)
{
  PanelPtr ptr{
    new Panel{&theme, {GUI_PANEL, ALIGN_TOP_LEFT, 0, {std::move(elems)}}}};
  return addPanel(std::move(ptr), x, y, align);
}

gx::PanelID gx::Gui::newPanel(const GuiTheme& theme, float x, float y,
                              AlignEnum align, const GuiElem& elems)
{
  PanelPtr ptr{
    new Panel{&theme, {GUI_PANEL, ALIGN_TOP_LEFT, 0, {elems}}}};
  return addPanel(std::move(ptr), x, y, align);
}
