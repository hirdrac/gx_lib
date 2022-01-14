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
  using PanelID = int;  // internal/external panel ID
  using ElemID = int;   // internal GuiElem ID
  using EventID = int;  // user specified event value

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
    GUI_PANEL, GUI_MENU_FRAME,
    GUI_LABEL, GUI_HLINE, GUI_VLINE, GUI_IMAGE,
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
    int64_t repeatDelay; // BUTTON_PRESS
    bool checkboxSet;    // CHECKBOX
    int itemNo;          // LISTSELECT, LISTSELECT_ITEM
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

  void update(Window& win);
    // process events & update drawLists

  [[nodiscard]] const DrawList& drawList() const { return _dl; }

  [[nodiscard]] EventID eventID() const { return _eventID; }
  [[nodiscard]] GuiElemType eventType() const { return _eventType; }
  [[nodiscard]] int64_t lastEventTime() const { return _eventTime; }
    // id/type of element triggering an event

  [[nodiscard]] bool needRedraw() const { return _needRedraw; }
    // true if GUI needs to be redrawn

  void setElemState(EventID eid, bool enable);
  void enableElem(EventID eid) { setElemState(eid, true); }
  void disableElem(EventID eid) { setElemState(eid, false); }
    // enable/disable event generating elements

  [[nodiscard]] std::string getText(EventID eid) const {
    const GuiElem* e = findElemByEventID(eid);
    return (e == nullptr) ? std::string() : e->text;
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

  std::vector<std::unique_ptr<Panel>> _panels;
  PanelID _lastPanelID = 0;
  PanelID _lastUniqueID = 0;

  DrawList _dl, _dl2;
  ElemID _hoverID = 0;
  ElemID _heldID = 0;
  ElemID _focusID = 0;
  ElemID _popupID = 0;
  EventID _eventID = 0;
  GuiElemType _heldType = GUI_NULL;
  GuiElemType _popupType = GUI_NULL;
  GuiElemType _eventType = GUI_NULL;
  int64_t _eventTime = 0;
  int64_t _heldTime = 0;
  int64_t _repeatDelay = -1;
  int64_t _lastCursorUpdate = 0;
  uint32_t _cursorBlinkTime = 0; // cached theme value
  float _heldX = 0, _heldY = 0;
  bool _cursorState = false;
  bool _needRender = true;
  bool _needRedraw = false;
  bool _textChanged = false;

  PanelID addPanel(
    std::unique_ptr<Panel> ptr, float x, float y, AlignEnum align);
  void layout(Panel& p, float x, float y, AlignEnum align);
  void processMouseEvent(Window& win);
  void processCharEvent(Window& win);
  void setFocusID(Window& win, ElemID id);
  void initElem(GuiElem& def);
  void deactivatePopups();
  void activatePopup(const GuiElem& def);
  bool drawElem(const Panel& p, GuiElem& def, DrawContext& dc, DrawContext& dc2,
                int64_t usec, const GuiTheme::Style* style) const;
  bool drawPopup(const Panel& p, GuiElem& def, DrawContext& dc, DrawContext& dc2,
                 int64_t usec) const;

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

  std::unique_ptr<Panel> removePanel(PanelID id) {
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
  std::unique_ptr<Panel> ptr{
    new Panel{&theme, {GUI_PANEL, ALIGN_TOP_LEFT, 0, {std::move(elems)}}}};
  return addPanel(std::move(ptr), x, y, align);
}

gx::PanelID gx::Gui::newPanel(const GuiTheme& theme, float x, float y,
                              AlignEnum align, const GuiElem& elems)
{
  std::unique_ptr<Panel> ptr{
    new Panel{&theme, {GUI_PANEL, ALIGN_TOP_LEFT, 0, {elems}}}};
  return addPanel(std::move(ptr), x, y, align);
}

namespace gx {
  // **** GuiElem functions ****

  // HFrame
  template<typename... Elems>
  inline GuiElem guiHFrame(const Elems&... elems)
  {
    return {GUI_HFRAME, ALIGN_TOP_LEFT, 0, {elems...}};
  }

  template<typename... Elems>
  inline GuiElem guiHFrame(AlignEnum align, const Elems&... elems)
  {
    return {GUI_HFRAME, align, 0, {elems...}};
  }

  // VFrame
  template<typename... Elems>
  inline GuiElem guiVFrame(const Elems&... elems)
  {
    return {GUI_VFRAME, ALIGN_TOP_LEFT, 0, {elems...}};
  }

  template<typename... Elems>
  inline GuiElem guiVFrame(AlignEnum align, const Elems&... elems)
  {
    return {GUI_VFRAME, align, 0, {elems...}};
  }

  // Spacer
  inline GuiElem guiSpacer(int16_t width, int16_t height)
  {
    GuiElem e{GUI_SPACER, ALIGN_CENTER, 0};
    e.spacer.left = width;
    e.spacer.top = height;
    e.spacer.right = 0;
    e.spacer.bottom = 0;
    return e;
  }

  inline GuiElem guiSpacer(AlignEnum align, int16_t width, int16_t height)
  {
    GuiElem e{GUI_SPACER, align, 0};
    e.spacer.left = width;
    e.spacer.top = height;
    e.spacer.right = 0;
    e.spacer.bottom = 0;
    return e;
  }

  inline GuiElem guiSpacer(int16_t left, int16_t top, int16_t right,
                           int16_t bottom, const GuiElem& elem)
  {
    GuiElem e{GUI_SPACER, ALIGN_CENTER, 0, {elem}};
    e.spacer.left = left;
    e.spacer.top = top;
    e.spacer.right = right;
    e.spacer.bottom = bottom;
    return e;
  }

  inline GuiElem guiSpacer(AlignEnum align, int16_t left, int16_t top,
                           int16_t right, int16_t bottom, const GuiElem& elem)
  {
    GuiElem e{GUI_SPACER, align, 0, {elem}};
    e.spacer.left = left;
    e.spacer.top = top;
    e.spacer.right = right;
    e.spacer.bottom = bottom;
    return e;
  }

  // Label
  inline GuiElem guiLabel(std::string_view text)
  {
    GuiElem e{GUI_LABEL, ALIGN_TOP_LEFT, 0};
    e.text = text;
    return e;
  }

  inline GuiElem guiLabel(AlignEnum align, std::string_view text)
  {
    GuiElem e{GUI_LABEL, align, 0};
    e.text = text;
    return e;
  }

  // HLine
  inline GuiElem guiHLine()
  {
    return {GUI_HLINE, ALIGN_HJUSTIFY, 0};
  }

  // VLine
  inline GuiElem guiVLine()
  {
    return {GUI_VLINE, ALIGN_VJUSTIFY, 0};
  }

  // Button
  // (triggered on button release)
  inline GuiElem guiButton(EventID id, const GuiElem& elem)
  {
    return {GUI_BUTTON, ALIGN_TOP_LEFT, id, {elem}};
  }

  inline GuiElem guiButton(EventID id, AlignEnum align, const GuiElem& elem)
  {
    return {GUI_BUTTON, align, id, {elem}};
  }

  inline GuiElem guiButton(EventID id, std::string_view text)
  {
    return {GUI_BUTTON, ALIGN_TOP_LEFT, id, {guiLabel(ALIGN_CENTER, text)}};
  }

  inline GuiElem guiButton(EventID id, AlignEnum align, std::string_view text)
  {
    return {GUI_BUTTON, align, id, {guiLabel(ALIGN_CENTER, text)}};
  }

  // ButtonPress
  // (triggered on initial button press)
  inline GuiElem guiButtonPress(EventID id, const GuiElem& elem)
  {
    GuiElem e{GUI_BUTTON_PRESS, ALIGN_TOP_LEFT, id, {elem}};
    e.repeatDelay = -1; // disabled
    return e;
  }

  inline GuiElem guiButtonPress(
    EventID id, AlignEnum align, const GuiElem& elem)
  {
    GuiElem e{GUI_BUTTON_PRESS, align, id, {elem}};
    e.repeatDelay = -1; // disabled
    return e;
  }

  inline GuiElem guiButtonPress(EventID id, std::string_view text)
  {
    GuiElem e{GUI_BUTTON_PRESS, ALIGN_TOP_LEFT, id,
              {guiLabel(ALIGN_CENTER, text)}};
    e.repeatDelay = -1; // disabled
    return e;
  }

  inline GuiElem guiButtonPress(
    EventID id, AlignEnum align, std::string_view text)
  {
    GuiElem e{GUI_BUTTON_PRESS, align, id, {guiLabel(ALIGN_CENTER, text)}};
    e.repeatDelay = -1; // disabled
    return e;
  }

  // ButtonPress with repeat if held
  inline GuiElem guiButtonHold(
    EventID id, int64_t repeat_delay, const GuiElem& elem)
  {
    GuiElem e{GUI_BUTTON_PRESS, ALIGN_TOP_LEFT, id, {elem}};
    e.repeatDelay = repeat_delay;
    return e;
  }

  inline GuiElem guiButtonHold(
    EventID id, AlignEnum align, int64_t repeat_delay, const GuiElem& elem)
  {
    GuiElem e{GUI_BUTTON_PRESS, align, id, {elem}};
    e.repeatDelay = repeat_delay;
    return e;
  }

  inline GuiElem guiButtonHold(
    EventID id, int64_t repeat_delay, std::string_view text)
  {
    GuiElem e{GUI_BUTTON_PRESS, ALIGN_TOP_LEFT, id,
              {guiLabel(ALIGN_CENTER, text)}};
    e.repeatDelay = repeat_delay;
    return e;
  }

  inline GuiElem guiButtonHold(
    EventID id, AlignEnum align, int64_t repeat_delay, std::string_view text)
  {
    GuiElem e{GUI_BUTTON_PRESS, align, id, {guiLabel(ALIGN_CENTER, text)}};
    e.repeatDelay = repeat_delay;
    return e;
  }

  // Checkbox
  inline GuiElem guiCheckbox(EventID id, bool set, const GuiElem& label)
  {
    GuiElem e{GUI_CHECKBOX, ALIGN_LEFT, id, {label}};
    e.checkboxSet = set;
    return e;
  }

  inline GuiElem guiCheckbox(
    EventID id, AlignEnum align, bool set, const GuiElem& label)
  {
    GuiElem e{GUI_CHECKBOX, align, id, {label}};
    e.checkboxSet = set;
    return e;
  }

  inline GuiElem guiCheckbox(EventID id, bool set, std::string_view label)
  {
    GuiElem e{GUI_CHECKBOX, ALIGN_TOP_LEFT, id, {guiLabel(ALIGN_LEFT, label)}};
    e.checkboxSet = set;
    return e;
  }

  inline GuiElem guiCheckbox(
    EventID id, AlignEnum align, bool set, std::string_view label)
  {
    GuiElem e{GUI_CHECKBOX, align, id, {guiLabel(ALIGN_LEFT, label)}};
    e.checkboxSet = set;
    return e;
  }

  // Menu
  template<typename... Elems>
  inline GuiElem guiMenu(std::string_view text, const Elems&... items)
  {
    return {GUI_MENU, ALIGN_TOP_LEFT, 0,
            {guiLabel(ALIGN_CENTER, text),
             GuiElem{GUI_MENU_FRAME, ALIGN_TOP_LEFT, 0,
                     {guiVFrame(items...)}}}};
  }

  inline GuiElem guiMenuItem(EventID id, std::string_view text)
  {
    return {GUI_MENU_ITEM, ALIGN_JUSTIFY, id,
            {guiLabel(ALIGN_CENTER_LEFT, text)}};
  }

  template<typename... Elems>
  inline GuiElem guiSubMenu(std::string_view text, const Elems&... items)
  {
    return {GUI_SUBMENU, ALIGN_JUSTIFY, 0,
            {guiLabel(ALIGN_CENTER_LEFT, text),
             GuiElem{GUI_MENU_FRAME, ALIGN_TOP_LEFT, 0,
                     {guiVFrame(items...)}}}};
  }

  // List Select
  template<typename... Elems>
  inline GuiElem guiListSelect(EventID id, const Elems&... items)
  {
    GuiElem e{GUI_LISTSELECT, ALIGN_TOP_LEFT, id,
              {GuiElem{},
               GuiElem{GUI_MENU_FRAME, ALIGN_TOP_LEFT, 0,
                       {guiVFrame(items...)}}}};
    e.itemNo = 0; // unset (default to first item)
    return e;
  }

  template<typename... Elems>
  inline GuiElem guiListSelect(EventID id, AlignEnum align,
                               const Elems&... items)
  {
    GuiElem e{GUI_LISTSELECT, align, id,
              {GuiElem{},
               GuiElem{GUI_MENU_FRAME, ALIGN_TOP_LEFT, 0,
                       {guiVFrame(items...)}}}};
    e.itemNo = 0; // unset (default to first item)
    return e;
  }

  inline GuiElem guiListSelectItem(int no, std::string_view text)
  {
    GuiElem e{GUI_LISTSELECT_ITEM, ALIGN_JUSTIFY, 0,
              {guiLabel(ALIGN_CENTER_LEFT, text)}};
    e.itemNo = no; // should be non-zero
    return e;
  }

  // Entry
  inline GuiElem guiEntry(EventID id, AlignEnum align, EntryType type,
                          float size, uint32_t maxLen, AlignEnum textAlign)
  {
    GuiElem e{GUI_ENTRY, align, id};
    e.entry.size = size;
    e.entry.maxLength = maxLen;
    e.entry.type = type;
    e.entry.align = textAlign;
    return e;
  }

  inline GuiElem guiTextEntry(
    EventID id, float size, uint32_t maxLen, AlignEnum textAlign = ALIGN_LEFT)
  {
    return guiEntry(id, ALIGN_TOP_LEFT, ENTRY_TEXT, size, maxLen, textAlign);
  }

  inline GuiElem guiTextEntry(
    EventID id, AlignEnum align, float size, uint32_t maxLen,
    AlignEnum textAlign = ALIGN_LEFT)
  {
    return guiEntry(id, align, ENTRY_TEXT, size, maxLen, textAlign);
  }

  inline GuiElem guiCardinalEntry(
    EventID id, float size, uint32_t maxLen, AlignEnum textAlign = ALIGN_LEFT)
  {
    return guiEntry(id, ALIGN_TOP_LEFT, ENTRY_CARDINAL, size, maxLen, textAlign);
  }

  inline GuiElem guiCardinalEntry(
    EventID id, AlignEnum align, float size, uint32_t maxLen,
    AlignEnum textAlign = ALIGN_LEFT)
  {
    return guiEntry(id, align, ENTRY_CARDINAL, size, maxLen, textAlign);
  }

  inline GuiElem guiIntegerEntry(
    EventID id, float size, uint32_t maxLen, AlignEnum textAlign = ALIGN_LEFT)
  {
    return guiEntry(id, ALIGN_TOP_LEFT, ENTRY_INTEGER, size, maxLen, textAlign);
  }

  inline GuiElem guiIntegerEntry(
    EventID id, AlignEnum align, float size, uint32_t maxLen,
    AlignEnum textAlign = ALIGN_LEFT)
  {
    return guiEntry(id, align, ENTRY_INTEGER, size, maxLen, textAlign);
  }

  inline GuiElem guiFloatEntry(
    EventID id, float size, uint32_t maxLen, AlignEnum textAlign = ALIGN_LEFT)
  {
    return guiEntry(id, ALIGN_TOP_LEFT, ENTRY_FLOAT, size, maxLen, textAlign);
  }

  inline GuiElem guiFloatEntry(
    EventID id, AlignEnum align, float size, uint32_t maxLen,
    AlignEnum textAlign = ALIGN_LEFT)
  {
    return guiEntry(id, align, ENTRY_FLOAT, size, maxLen, textAlign);
  }

  inline GuiElem guiPasswordEntry(
    EventID id, float size, uint32_t maxLen, AlignEnum textAlign = ALIGN_LEFT)
  {
    return guiEntry(id, ALIGN_TOP_LEFT, ENTRY_PASSWORD, size, maxLen, textAlign);
  }

  inline GuiElem guiPasswordEntry(
    EventID id, AlignEnum align, float size, uint32_t maxLen,
    AlignEnum textAlign = ALIGN_LEFT)
  {
    return guiEntry(id, align, ENTRY_PASSWORD, size, maxLen, textAlign);
  }

  // Image
  inline GuiElem guiImage(float w, float h, TextureID tid, Vec2 t0, Vec2 t1)
  {
    GuiElem e{GUI_IMAGE, ALIGN_TOP_LEFT, 0};
    e.image.width = w;
    e.image.height = h;
    e.image.texId = tid;
    e.image.texCoord0 = t0;
    e.image.texCoord1 = t1;
    return e;
  }

  // TitleBar
  inline GuiElem guiTitleBar()
  {
    return {GUI_TITLEBAR, ALIGN_HJUSTIFY, 0};
  }

  inline GuiElem guiVTitleBar()
  {
    return {GUI_TITLEBAR, ALIGN_VJUSTIFY, 0};
  }

  inline GuiElem guiTitleBar(std::string_view text)
  {
    return {GUI_TITLEBAR, ALIGN_HJUSTIFY, 0, {guiLabel(ALIGN_CENTER, text)}};
  }
}
