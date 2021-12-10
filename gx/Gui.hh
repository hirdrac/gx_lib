//
// gx/Gui.hh
// Copyright (C) 2021 Richard Bradley
//
// Graphical user interface rendering & event handling
//

// TODO: sub menus
// TODO: menu item key short-cuts
// TODO: list select (combo box)

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
  using PanelID = int;
  using EventID = int;

  class Window;
  class DrawContext;
  struct TextFormatting;
  class Font;
  class Gui;
  struct GuiElem;

  enum GuiElemType {
    GUI_NULL = 0,

    // layout types
    GUI_HFRAME, GUI_VFRAME,

    // draw types
    GUI_BACKGROUND, GUI_LABEL, GUI_HLINE, GUI_VLINE, GUI_IMAGE,

    // event types
    GUI_BUTTON,        // activated on release
    GUI_BUTTON_PRESS,  // activated once on press
    GUI_BUTTON_HOLD,   // activated continuously on hold&press
    GUI_CHECKBOX,      // toggle value on release
    GUI_MENU,          // button hold or release on menu opens menu
    GUI_MENU_ITEM,     // activated on press or release
    GUI_ENTRY,         // activated if changed on enter/tab/click-away
  };

  enum EntryType {
    ENTRY_TEXT,     // all characters valid
    ENTRY_CARDINAL, // positive integer
    ENTRY_INTEGER,  // positive/negitive integer
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
  EventID id;

  // elem type specific properties
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
    bool checkbox_set;
    EntryProps entry;
    ImageProps image;
  };

  // layout state
  float _x = 0, _y = 0, _w = 0, _h = 0; // layout position/size
  bool _active = false;                 // popup/menu enabled

  GuiElem(GuiElemType t, AlignEnum a, EventID i)
    : type{t}, align{a}, id{i} { }
  GuiElem(GuiElemType t, AlignEnum a, EventID i,
          std::initializer_list<GuiElem> x)
    : elems{x}, type{t}, align{a}, id{i} { }
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

  void update(Window& win);
    // process events & update drawLists

  [[nodiscard]] const DrawList& drawList() const { return _dl; }
  [[nodiscard]] EventID eventID() const { return _eventID; }
    // id of element triggering an event
  [[nodiscard]] bool needRedraw() const { return _needRedraw; }
    // true if GUI needs to be redrawn

  [[nodiscard]] std::string getText(EventID id) const {
    const GuiElem* e = findElem(id);
    return (e == nullptr) ? std::string() : e->text;
  }

  [[nodiscard]] bool getBool(EventID id) const {
    const GuiElem* e = findElem(id);
    return (e == nullptr || e->type != GUI_CHECKBOX) ? false : e->checkbox_set;
  }

  bool setText(EventID id, std::string_view text);
  bool setBool(EventID id, bool val);

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
  EventID _hoverID = 0;
  EventID _heldID = 0;
  EventID _focusID = 0;
  EventID _eventID = 0;
  GuiElemType _heldType = GUI_NULL;
  int64_t _lastCursorUpdate = 0;
  uint32_t _cursorBlinkTime = 0;
  bool _cursorState = false;
  bool _needRender = true;
  bool _needRedraw = false;
  bool _textChanged = false;
  bool _popupActive = false;

  PanelID addPanel(
    std::unique_ptr<Panel> ptr, float x, float y, AlignEnum align);
  void layout(Panel& p, float x, float y, AlignEnum align);
  void processMouseEvent(Window& win);
  void processCharEvent(Window& win);
  void setFocusID(Window& win, EventID id);
  void initElem(GuiElem& def);
  void deactivatePopup();
  void drawElem(
    DrawContext& dc, DrawContext& dc2, const TextFormatting& tf,
    const GuiElem& def, const Panel& panel, const GuiTheme::Style* style) const;
  void drawPopup(
    DrawContext& dc, DrawContext& dc2, const TextFormatting& tf,
    const GuiElem& def, const Panel& panel) const;

  [[nodiscard]] Panel* findPanel(PanelID id);
  [[nodiscard]] const Panel* findPanel(PanelID id) const;
  [[nodiscard]] GuiElem* findElem(EventID id);
  [[nodiscard]] const GuiElem* findElem(EventID id) const;
  [[nodiscard]] GuiElem* findNextElem(EventID id, GuiElemType type = GUI_NULL);
  [[nodiscard]] GuiElem* findPrevElem(EventID id, GuiElemType type = GUI_NULL);
};

// **** Inline Implementations ****
gx::PanelID gx::Gui::newPanel(const GuiTheme& theme, float x, float y,
                              AlignEnum align, GuiElem&& elems)
{
  std::unique_ptr<Panel> ptr{
    new Panel{&theme, {GUI_BACKGROUND, ALIGN_TOP_LEFT, 0, {std::move(elems)}}}};
  return addPanel(std::move(ptr), x, y, align);
}

gx::PanelID gx::Gui::newPanel(const GuiTheme& theme, float x, float y,
                              AlignEnum align, const GuiElem& elems)
{
  std::unique_ptr<Panel> ptr{
    new Panel{&theme, {GUI_BACKGROUND, ALIGN_TOP_LEFT, 0, {elems}}}};
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
    return {GUI_BUTTON_PRESS, ALIGN_TOP_LEFT, id, {elem}};
  }

  inline GuiElem guiButtonPress(
    EventID id, AlignEnum align, const GuiElem& elem)
  {
    return {GUI_BUTTON_PRESS, align, id, {elem}};
  }

  inline GuiElem guiButtonPress(EventID id, std::string_view text)
  {
    return {GUI_BUTTON_PRESS, ALIGN_TOP_LEFT, id,
            {guiLabel(ALIGN_CENTER, text)}};
  }

  inline GuiElem guiButtonPress(
    EventID id, AlignEnum align, std::string_view text)
  {
    return {GUI_BUTTON_PRESS, align, id, {guiLabel(ALIGN_CENTER, text)}};
  }

  // ButtonHold
  // (triggered repeatedly while held)
  inline GuiElem guiButtonHold(EventID id, const GuiElem& elem)
  {
    return {GUI_BUTTON_HOLD, ALIGN_TOP_LEFT, id, {elem}};
  }

  inline GuiElem guiButtonHold(
    EventID id, AlignEnum align, const GuiElem& elem)
  {
    return {GUI_BUTTON_HOLD, align, id, {elem}};
  }

  inline GuiElem guiButtonHold(EventID id, std::string_view text)
  {
    return {GUI_BUTTON_HOLD, ALIGN_TOP_LEFT, id,
            {guiLabel(ALIGN_CENTER, text)}};
  }

  inline GuiElem guiButtonHold(
    EventID id, AlignEnum align, std::string_view text)
  {
    return {GUI_BUTTON_HOLD, align, id, {guiLabel(ALIGN_CENTER, text)}};
  }

  // Checkbox
  inline GuiElem guiCheckbox(EventID id, bool set, const GuiElem& label)
  {
    GuiElem e{GUI_CHECKBOX, ALIGN_LEFT, id, {label}};
    e.checkbox_set = set;
    return e;
  }

  inline GuiElem guiCheckbox(
    EventID id, AlignEnum align, bool set, const GuiElem& label)
  {
    GuiElem e{GUI_CHECKBOX, align, id, {label}};
    e.checkbox_set = set;
    return e;
  }

  inline GuiElem guiCheckbox(EventID id, bool set, std::string_view label)
  {
    GuiElem e{GUI_CHECKBOX, ALIGN_TOP_LEFT, id, {guiLabel(ALIGN_LEFT, label)}};
    e.checkbox_set = set;
    return e;
  }

  inline GuiElem guiCheckbox(
    EventID id, AlignEnum align, bool set, std::string_view label)
  {
    GuiElem e{GUI_CHECKBOX, align, id, {guiLabel(ALIGN_LEFT, label)}};
    e.checkbox_set = set;
    return e;
  }

  // Menu
  template<typename... Elems>
  inline GuiElem guiMenu(std::string_view text, const Elems&... items)
  {
    return {GUI_MENU, ALIGN_TOP_LEFT, 0,
            {guiLabel(ALIGN_CENTER, text),
             GuiElem{GUI_BACKGROUND, ALIGN_TOP_LEFT, 0,
                     {guiVFrame(items...)}}}};
  }

  inline GuiElem guiMenuItem(EventID id, std::string_view text)
  {
    return {GUI_MENU_ITEM, ALIGN_JUSTIFY, id,
            {guiLabel(ALIGN_CENTER_LEFT, text)}};
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
}
