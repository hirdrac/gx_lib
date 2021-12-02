//
// gx/Gui.hh
// Copyright (C) 2021 Richard Bradley
//
// Graphical user interface rendering & event handling
//

// TODO: sub menus
// TODO: menu item key short-cuts
// TODO: list select (combo box)
// TODO: modal dialog

#pragma once
#include "DrawList.hh"
#include "Align.hh"
#include "Color.hh"
#include "Types.hh"
#include <string_view>
#include <vector>
#include <initializer_list>


namespace gx {
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
  int id;

  // elem type specific properties
  struct EntryProps {
    float size; // width in characters
    int maxLength;
    EntryType type;
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
  uint16_t textSpacing = 0;
  uint16_t lineWidth = 2;
};


class gx::Gui
{
 public:
  Gui(const GuiElem& elems)
    : _rootElem{GUI_BACKGROUND, ALIGN_TOP_LEFT, 0, {elems}} { init(_rootElem); }

  void layout(const GuiTheme& theme, float x, float y, AlignEnum align);
  void update(Window& win);

  [[nodiscard]] const DrawListMap& drawLists() const { return _dlm; }
  [[nodiscard]] float width() const { return _rootElem._w; }
  [[nodiscard]] float height() const { return _rootElem._h; }

  [[nodiscard]] int eventID() const { return _eventID; }
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
  const GuiTheme* _theme = nullptr;
  GuiElem _rootElem;
  DrawListMap _dlm;
  Rect _layout;
  int _hoverID = 0;
  int _heldID = 0;
  int _focusID = 0;
  int _eventID = 0;
  GuiElemType _heldType = GUI_NULL;
  int64_t _lastCursorUpdate = 0;
  int _uniqueID = -1;
  bool _cursorState = false;
  bool _needLayout = true;
  bool _needRender = true;
  bool _needRedraw = false;
  bool _textChanged = false;
  bool _popupActive = false;

  void processMouseEvent(Window& win);
  void processCharEvent(Window& win);
  void setFocusID(Window& win, int id);
  void init(GuiElem& def);
  void calcSize(GuiElem& def);
  void calcPos(GuiElem& e, float left, float top, float right, float bottom);
  void drawElem(
    DrawContext& dc, DrawContext& dc2, const TextFormatting& tf,
    const GuiElem& def, const GuiTheme::Style* style) const;
  void drawPopup(
    DrawContext& dc, DrawContext& dc2, const TextFormatting& tf,
    const GuiElem& def) const;

  [[nodiscard]] GuiElem* findElem(int id);
  [[nodiscard]] const GuiElem* findElem(int id) const;
  [[nodiscard]] GuiElem* findNextElem(int id, GuiElemType type = GUI_NULL);
  [[nodiscard]] GuiElem* findPrevElem(int id, GuiElemType type = GUI_NULL);
};


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
  inline GuiElem guiButton(int eventID, const GuiElem& elem)
  {
    return {GUI_BUTTON, ALIGN_TOP_LEFT, eventID, {elem}};
  }

  inline GuiElem guiButton(int eventID, AlignEnum align, const GuiElem& elem)
  {
    return {GUI_BUTTON, align, eventID, {elem}};
  }

  inline GuiElem guiButton(int eventID, std::string_view text)
  {
    return {GUI_BUTTON, ALIGN_TOP_LEFT, eventID, {guiLabel(ALIGN_CENTER, text)}};
  }

  inline GuiElem guiButton(int eventID, AlignEnum align, std::string_view text)
  {
    return {GUI_BUTTON, align, eventID, {guiLabel(ALIGN_CENTER, text)}};
  }

  // ButtonPress
  // (triggered on initial button press)
  inline GuiElem guiButtonPress(int eventID, const GuiElem& elem)
  {
    return {GUI_BUTTON_PRESS, ALIGN_TOP_LEFT, eventID, {elem}};
  }

  inline GuiElem guiButtonPress(
    int eventID, AlignEnum align, const GuiElem& elem)
  {
    return {GUI_BUTTON_PRESS, align, eventID, {elem}};
  }

  inline GuiElem guiButtonPress(int eventID, std::string_view text)
  {
    return {GUI_BUTTON_PRESS, ALIGN_TOP_LEFT, eventID,
            {guiLabel(ALIGN_CENTER, text)}};
  }

  inline GuiElem guiButtonPress(
    int eventID, AlignEnum align, std::string_view text)
  {
    return {GUI_BUTTON_PRESS, align, eventID, {guiLabel(ALIGN_CENTER, text)}};
  }

  // ButtonHold
  // (triggered repeatedly while held)
  inline GuiElem guiButtonHold(int eventID, const GuiElem& elem)
  {
    return {GUI_BUTTON_HOLD, ALIGN_TOP_LEFT, eventID, {elem}};
  }

  inline GuiElem guiButtonHold(
    int eventID, AlignEnum align, const GuiElem& elem)
  {
    return {GUI_BUTTON_HOLD, align, eventID, {elem}};
  }

  inline GuiElem guiButtonHold(int eventID, std::string_view text)
  {
    return {GUI_BUTTON_HOLD, ALIGN_TOP_LEFT, eventID,
            {guiLabel(ALIGN_CENTER, text)}};
  }

  inline GuiElem guiButtonHold(
    int eventID, AlignEnum align, std::string_view text)
  {
    return {GUI_BUTTON_HOLD, align, eventID, {guiLabel(ALIGN_CENTER, text)}};
  }

  // Checkbox
  inline GuiElem guiCheckbox(int eventID, bool set, const GuiElem& label)
  {
    GuiElem e{GUI_CHECKBOX, ALIGN_LEFT, eventID, {label}};
    e.checkbox_set = set;
    return e;
  }

  inline GuiElem guiCheckbox(
    int eventID, AlignEnum align, bool set, const GuiElem& label)
  {
    GuiElem e{GUI_CHECKBOX, align, eventID, {label}};
    e.checkbox_set = set;
    return e;
  }

  inline GuiElem guiCheckbox(int eventID, bool set, std::string_view label)
  {
    GuiElem e{GUI_CHECKBOX, ALIGN_TOP_LEFT, eventID,
              {guiLabel(ALIGN_LEFT, label)}};
    e.checkbox_set = set;
    return e;
  }

  inline GuiElem guiCheckbox(
    int eventID, AlignEnum align, bool set, std::string_view label)
  {
    GuiElem e{GUI_CHECKBOX, align, eventID, {guiLabel(ALIGN_LEFT, label)}};
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

  inline GuiElem guiMenuItem(int eventID, std::string_view text)
  {
    return {GUI_MENU_ITEM, ALIGN_JUSTIFY, eventID,
            {guiLabel(ALIGN_CENTER_LEFT, text)}};
  }

  // Entry
  inline GuiElem guiTextEntry(int eventID, float size, int maxLength)
  {
    GuiElem e{GUI_ENTRY, ALIGN_TOP_LEFT, eventID};
    e.entry.size = size;
    e.entry.maxLength = maxLength;
    e.entry.type = ENTRY_TEXT;
    return e;
  }

  inline GuiElem guiCardinalEntry(int eventID, float size, int maxLength)
  {
    GuiElem e{GUI_ENTRY, ALIGN_TOP_LEFT, eventID};
    e.entry.size = size;
    e.entry.maxLength = maxLength;
    e.entry.type = ENTRY_CARDINAL;
    return e;
  }

  inline GuiElem guiIntegerEntry(int eventID, float size, int maxLength)
  {
    GuiElem e{GUI_ENTRY, ALIGN_TOP_LEFT, eventID};
    e.entry.size = size;
    e.entry.maxLength = maxLength;
    e.entry.type = ENTRY_INTEGER;
    return e;
  }

  inline GuiElem guiFloatEntry(int eventID, float size, int maxLength)
  {
    GuiElem e{GUI_ENTRY, ALIGN_TOP_LEFT, eventID};
    e.entry.size = size;
    e.entry.maxLength = maxLength;
    e.entry.type = ENTRY_FLOAT;
    return e;
  }

  inline GuiElem guiPasswordEntry(int eventID, float size, int maxLength)
  {
    GuiElem e{GUI_ENTRY, ALIGN_TOP_LEFT, eventID};
    e.entry.size = size;
    e.entry.maxLength = maxLength;
    e.entry.type = ENTRY_PASSWORD;
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
