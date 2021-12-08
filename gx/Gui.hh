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
#include "DrawList.hh"
#include "Align.hh"
#include "Color.hh"
#include "Types.hh"
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
  struct GuiTheme;

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
    int maxLength;
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

  GuiElem(GuiElemType t, AlignEnum a, int i)
    : type{t}, align{a}, id{i} { }
  GuiElem(GuiElemType t, AlignEnum a, int i, std::initializer_list<GuiElem> x)
    : elems{x}, type{t}, align{a}, id{i} { }
};


struct gx::GuiTheme
{
  struct Style {
    RGBA8 textColor;
    RGBA8 backgroundColor;
    RGBA8 edgeColor;
  };

  const Font* font = nullptr;
  Style panel = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.2f,.2f,.2f,1.0f), 0};

  Style button = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.4f,.4f,.4f,1.0f), 0};
  Style buttonHover = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.8f,.4f,.4f,1.0f), 0};
  Style buttonPress = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.8f,.8f,.8f,1.0f), 0};
  Style buttonHold = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.6f,.6f,.6f,1.0f), 0};

  Style checkbox = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.4f,.4f,.4f,1.0f), 0};
  Style checkboxHover = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.8f,.4f,.4f,1.0f), 0};
  Style checkboxPress = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.8f,.8f,.8f,1.0f), 0};
  Style checkboxHold = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.6f,.6f,.6f,1.0f), 0};

  Style menuButton = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), 0, 0};
  Style menuButtonHover = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.8f,.4f,.4f,1.0f), 0};
  Style menuButtonOpen = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.6f,.6f,.6f,1.0f), 0};
  Style menuFrame = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(0.0f,0.0f,0.0f,1.0f), 0};
  Style menuItemSelect = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.8f,.8f,.8f,1.0f), 0};

  Style entry = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(0.0f,0.0f,.2f,1.0f), 0};
  Style entryFocus = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.1f,.1f,.3f,1.0f), 0};

  uint16_t entryLeftMargin = 4;
  uint16_t entryRightMargin = 4;
  uint16_t entryTopMargin = 2;
  uint16_t entryBottomMargin = 2;

  int32_t checkCode = 'X';
  int16_t checkXOffset = 0;
  int16_t checkYOffset = 2;

  RGBA8 cursorColor = packRGBA8(1.0f,1.0f,.6f,1.0f);
  uint32_t cursorBlinkTime = 400000; // 1 sec
  uint16_t cursorWidth = 3;

  uint16_t border = 6;
  uint16_t frameSpacing = 4;
  uint16_t textSpacing = 1;
  uint16_t lineWidth = 2;
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

  void update(Window& win);
    // process events & update drawLists

  [[nodiscard]] const DrawList& drawList() const { return _dl; }
  [[nodiscard]] EventID eventID() const { return _eventID; }
    // id of element triggering an event
  [[nodiscard]] bool needRedraw() const { return _needRedraw; }
    // true if GUI needs to be redrawn

  [[nodiscard]] std::string getText(int id) const {
    const GuiElem* e = findElem(id);
    return (e == nullptr) ? std::string() : e->text;
  }

  [[nodiscard]] bool getBool(int id) const {
    const GuiElem* e = findElem(id);
    return (e == nullptr || e->type != GUI_CHECKBOX) ? false : e->checkbox_set;
  }

  bool setText(int id, std::string_view text);
  bool setBool(int id, bool val);

 private:
  struct Panel {
    const GuiTheme* theme;
    GuiElem root;
    PanelID id;

    Rect layout{};
    bool needLayout = true;
  };

  std::vector<std::unique_ptr<Panel>> _panels;
  PanelID _lastPanelID = 0;
  PanelID _lastUniqueID = 0;

  DrawList _dl, _dl2;
  int _hoverID = 0;
  int _heldID = 0;
  int _focusID = 0;
  EventID _eventID = 0;
  GuiElemType _heldType = GUI_NULL;
  int64_t _lastCursorUpdate = 0;
  uint32_t _cursorBlinkTime = 0;
  bool _cursorState = false;
  bool _needRender = true;
  bool _needRedraw = false;
  bool _textChanged = false;
  bool _popupActive = false;

  void layout(Panel& p, float x, float y, AlignEnum align);
  void processMouseEvent(Window& win);
  void processCharEvent(Window& win);
  void setFocusID(Window& win, int id);
  void init(GuiElem& def);
  void deactivatePopup();
  void drawElem(
    DrawContext& dc, DrawContext& dc2, const TextFormatting& tf,
    const GuiElem& def, const Panel& panel, const GuiTheme::Style* style) const;
  void drawPopup(
    DrawContext& dc, DrawContext& dc2, const TextFormatting& tf,
    const GuiElem& def, const Panel& panel) const;

  [[nodiscard]] Panel* findPanel(int id);
  [[nodiscard]] GuiElem* findElem(int id);
  [[nodiscard]] const GuiElem* findElem(int id) const;
  [[nodiscard]] GuiElem* findNextElem(int id, GuiElemType type = GUI_NULL);
  [[nodiscard]] GuiElem* findPrevElem(int id, GuiElemType type = GUI_NULL);
};

// **** Inline Implementations ****
gx::PanelID gx::Gui::newPanel(const GuiTheme& theme, float x, float y,
                              AlignEnum align, GuiElem&& elems)
{
  PanelID id = ++_lastPanelID;
  std::unique_ptr<Panel> ptr{new Panel{&theme, {GUI_BACKGROUND, ALIGN_TOP_LEFT, 0, {std::move(elems)}}, id}};
  init(ptr->root);
  layout(*ptr, x, y, align);
  _panels.push_back(std::move(ptr));
  return id;
}

gx::PanelID gx::Gui::newPanel(const GuiTheme& theme, float x, float y,
                              AlignEnum align, const GuiElem& elems)
{
  PanelID id = ++_lastPanelID;
  std::unique_ptr<Panel> ptr{new Panel{&theme, {GUI_BACKGROUND, ALIGN_TOP_LEFT, 0, {elems}}, id}};
  init(ptr->root);
  layout(*ptr, x, y, align);
  _panels.push_back(std::move(ptr));
  return id;
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
  inline GuiElem guiTextEntry(
    EventID id, float size, int maxLength, AlignEnum textAlign = ALIGN_LEFT)
  {
    GuiElem e{GUI_ENTRY, ALIGN_TOP_LEFT, id};
    e.entry.size = size;
    e.entry.maxLength = maxLength;
    e.entry.type = ENTRY_TEXT;
    e.entry.align = textAlign;
    return e;
  }

  inline GuiElem guiTextEntry(
    EventID id, AlignEnum align, float size, int maxLength,
    AlignEnum textAlign = ALIGN_LEFT)
  {
    GuiElem e{GUI_ENTRY, align, id};
    e.entry.size = size;
    e.entry.maxLength = maxLength;
    e.entry.type = ENTRY_TEXT;
    e.entry.align = textAlign;
    return e;
  }

  inline GuiElem guiCardinalEntry(
    EventID id, float size, int maxLength, AlignEnum textAlign = ALIGN_LEFT)
  {
    GuiElem e{GUI_ENTRY, ALIGN_TOP_LEFT, id};
    e.entry.size = size;
    e.entry.maxLength = maxLength;
    e.entry.type = ENTRY_CARDINAL;
    e.entry.align = textAlign;
    return e;
  }

  inline GuiElem guiCardinalEntry(
    EventID id, AlignEnum align, float size, int maxLength,
    AlignEnum textAlign = ALIGN_LEFT)
  {
    GuiElem e{GUI_ENTRY, align, id};
    e.entry.size = size;
    e.entry.maxLength = maxLength;
    e.entry.type = ENTRY_CARDINAL;
    e.entry.align = textAlign;
    return e;
  }

  inline GuiElem guiIntegerEntry(
    EventID id, float size, int maxLength, AlignEnum textAlign = ALIGN_LEFT)
  {
    GuiElem e{GUI_ENTRY, ALIGN_TOP_LEFT, id};
    e.entry.size = size;
    e.entry.maxLength = maxLength;
    e.entry.type = ENTRY_INTEGER;
    e.entry.align = textAlign;
    return e;
  }

  inline GuiElem guiIntegerEntry(
    EventID id, AlignEnum align, float size, int maxLength,
    AlignEnum textAlign = ALIGN_LEFT)
  {
    GuiElem e{GUI_ENTRY, align, id};
    e.entry.size = size;
    e.entry.maxLength = maxLength;
    e.entry.type = ENTRY_INTEGER;
    e.entry.align = textAlign;
    return e;
  }

  inline GuiElem guiFloatEntry(
    EventID id, float size, int maxLength, AlignEnum textAlign = ALIGN_LEFT)
  {
    GuiElem e{GUI_ENTRY, ALIGN_TOP_LEFT, id};
    e.entry.size = size;
    e.entry.maxLength = maxLength;
    e.entry.type = ENTRY_FLOAT;
    e.entry.align = textAlign;
    return e;
  }

  inline GuiElem guiFloatEntry(
    EventID id, AlignEnum align, float size, int maxLength,
    AlignEnum textAlign = ALIGN_LEFT)
  {
    GuiElem e{GUI_ENTRY, align, id};
    e.entry.size = size;
    e.entry.maxLength = maxLength;
    e.entry.type = ENTRY_FLOAT;
    e.entry.align = textAlign;
    return e;
  }

  inline GuiElem guiPasswordEntry(
    EventID id, float size, int maxLength, AlignEnum textAlign = ALIGN_LEFT)
  {
    GuiElem e{GUI_ENTRY, ALIGN_TOP_LEFT, id};
    e.entry.size = size;
    e.entry.maxLength = maxLength;
    e.entry.type = ENTRY_PASSWORD;
    e.entry.align = textAlign;
    return e;
  }

  inline GuiElem guiPasswordEntry(
    EventID id, AlignEnum align, float size, int maxLength,
    AlignEnum textAlign = ALIGN_LEFT)
  {
    GuiElem e{GUI_ENTRY, align, id};
    e.entry.size = size;
    e.entry.maxLength = maxLength;
    e.entry.type = ENTRY_PASSWORD;
    e.entry.align = textAlign;
    return e;
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
