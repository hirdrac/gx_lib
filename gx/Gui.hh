//
// gx/Gui.hh
// Copyright (C) 2024 Richard Bradley
//
// Graphical user interface rendering & event handling
//

#pragma once
#include "GuiElem.hh"
#include "GuiTheme.hh"
#include "DrawList.hh"
#include "Align.hh"
#include "Types.hh"
#include <string>
#include <string_view>
#include <vector>
#include <memory>


namespace gx {
  using PanelID = int32_t;  // internal/external panel ID

  class Window;
  class DrawContext;
  class Font;
  class Gui;
  struct GuiEvent;

  // panel flags
  constexpr int PANEL_FLOATING = 1;
}


struct gx::GuiEvent
{
  int64_t time;
  PanelID pid;
  EventID eid;
  int item_no;
  GuiElemType type;

  [[nodiscard]] explicit operator bool() const { return eid != 0; }
};


class gx::Gui
{
 public:
  template<class ElemT>
  PanelID newPanel(const GuiTheme& theme, float x, float y,
                   AlignEnum align, int flags, ElemT&& elems);

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

  [[nodiscard]] const GuiEvent& event() const { return _event; }
    // event details from last update

  [[nodiscard]] const DrawList& drawList() const { return _data; }
  [[nodiscard]] bool needRedraw() const { return _needRedraw; }
    // render data & flag if data was changed last update

  void setBGColor(float r, float g, float b) {
    _bgColor = packRGBA8(r,g,b,1.0f); }
  void setBGColor(const Color& c) {
    _bgColor = packRGBA8(c); }
  void setBGColor(RGBA8 c) {
    _bgColor = c; }

  void setElemState(PanelID pid, EventID eid, bool enable);
  void enableElem(PanelID pid, EventID eid) { setElemState(pid, eid, true); }
  void disableElem(PanelID pid, EventID eid) { setElemState(pid, eid, false); }
    // enable/disable event generating elements
    // TODO: add versions that also take item_no for menu/listselect items
    //  (calling w/o item_no should enable/disable entire menu/listselect)

  void setAllElemState(PanelID pid, bool enable);
  void enableAllElem(PanelID pid = 0) { setAllElemState(pid, true); }
  void disableAllElem(PanelID pid = 0) { setAllElemState(pid, false); }
    // enable/disable all event generating elements
    // (optionally just for a specific panel if id is non-zero)

  [[nodiscard]] std::string getText(PanelID pid, EventID eid) const {
    const GuiElem* e = findEventElem(pid, eid);
    switch (e->type) {
      case GUI_LABEL: case GUI_VLABEL:
        return e->label().text;
      case GUI_ENTRY:
        return e->entry().text;
      default:
        return {};
    }
  }

  [[nodiscard]] bool getBool(PanelID pid, EventID eid) const {
    const GuiElem* e = findEventElem(pid, eid);
    return (e == nullptr || e->type != GUI_CHECKBOX) ? false : e->checkbox().set;
  }

  [[nodiscard]] int getItemNo(PanelID pid, EventID eid) const {
    const GuiElem* e = findEventElem(pid, eid);
    return (e == nullptr || e->type != GUI_LISTSELECT) ? 0 : e->item().no;
  }

  [[nodiscard]] std::string eventText() const {
    return getText(_event.pid, _event.eid); }
  [[nodiscard]] bool eventBool() const {
    return getBool(_event.pid, _event.eid); }
  [[nodiscard]] int eventItemNo() const {
    return getItemNo(_event.pid, _event.eid); }

  bool setText(PanelID pid, EventID eid, std::string_view text);
  bool setBool(PanelID pid, EventID eid, bool val);
  bool setItemNo(PanelID pid, EventID eid, int itemNo);

  // TODO: methods to update menu/listselect items

 private:
  struct Panel {
    // set at creation
    GuiElem root;
    const GuiTheme* theme;
    int flags;

    // other attributes
    PanelID id = 0;
    Rect layout{};
    bool needLayout = false;
  };

  // element definition
  using PanelPtr = std::unique_ptr<Panel>;
  std::vector<PanelPtr> _panels;
  ElemID _lastElemID = 0;

  // current state
  DrawList _data;
  RGBA8 _bgColor = 0;
  ElemID _hoverID = 0;
  ElemID _focusID = 0;

  ElemID _popupID = 0;
  GuiElemType _popupType = GUI_NULL;

  GuiEvent _event{}, _event2{};

  int64_t _lastClickTime = 0;
  int _clickCount = 0;

  ElemID _heldID = 0;
  GuiElemType _heldType = GUI_NULL;
  Vec2 _heldPt;
  int64_t _heldTime = 0;
  int64_t _repeatDelay = -1;        // negative value disables repeat
  std::size_t _focusCursorPos = 0;  // character pos of cursor
  std::size_t _focusRangeStart = 0; // start selected range from cursorPos
    // if cursorPos != rangeStart, there is a selected range from
    //   min(pos,rs) to max(pos,rs)
  float _focusEntryOffset = 0;      // text render offset because of cursor

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
  void addEntryChar(GuiElem& e, int32_t code);
  void setFocus(Window& win, const GuiElem* e);
  void initElem(GuiElem& def);
  void deactivatePopups();
  void activatePopup(Panel& p, const GuiElem& def);
  bool drawElem(
    Window& win, Panel& p, GuiElem& def, DrawContext& dc, DrawContext& dc2,
    const GuiTheme::Style* style);
  bool drawPopup(Window& win, Panel& p, GuiElem& def,
                 DrawContext& dc, DrawContext& dc2);

  [[nodiscard]] std::pair<Panel*,GuiElem*> findElem(ElemID id);
  [[nodiscard]] GuiElem* findEventElem(PanelID pid, EventID eid);
  [[nodiscard]] const GuiElem* findEventElem(PanelID pid, EventID eid) const;

  void clearHeld() {
    _heldID = 0;
    _heldType = GUI_NULL;
    _heldTime = 0;
    _repeatDelay = -1;
  }

  void addEvent(const Panel& p, const GuiElem& e, int item_no, int64_t t);
  PanelPtr removePanel(PanelID id);
};


// **** Inline Implementations ****
template<class ElemT>
gx::PanelID gx::Gui::newPanel(
  const GuiTheme& theme, float x, float y, AlignEnum align, int flags,
  ElemT&& elems)
{
  PanelPtr ptr{new Panel{
      {GUI_PANEL, ALIGN_TOP_LEFT, 0, {std::forward<ElemT>(elems)}},
      &theme, flags}};
  return addPanel(std::move(ptr), x, y, align);
}
