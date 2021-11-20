//
// gx/Gui.hh
// Copyright (C) 2021 Richard Bradley
//
// Graphical user interface rendering & event handling
//

// TODO: sub menus
// TODO: menu item key short-cuts
// TODO: list select (combo box)
// TODO: theme text colors for button state & entry text
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

    // layout/draw types
    GUI_HFRAME, GUI_VFRAME,
    GUI_LABEL, GUI_HLINE, GUI_VLINE, GUI_IMAGE,
    GUI_MENU, GUI_MENU_HFRAME, GUI_MENU_VFRAME,

    // event types
    GUI_BUTTON,        // activated on release
    GUI_BUTTON_PRESS,  // activated once on press
    GUI_BUTTON_HOLD,   // activated continuously on hold&press
    GUI_MENU_ITEM,     // activated on release
    GUI_ENTRY,         // activated if changed on enter/tab/click-away
  };

  enum ButtonState {
    BSTATE_NONE = 0,  // no button
    BSTATE_NORMAL,    // normal button
    BSTATE_HOVER,     // pointer over button
    BSTATE_PRESSED,   // pointer over button with mouse left button pressed
    BSTATE_HELD_ONLY  // previously pressed & mouse left button still held
                      //   but pointer not over button
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
  GuiElemType type;
  std::string text;  // label text
  AlignEnum align = ALIGN_TOP_LEFT;
  int id = 0;

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
    EntryProps entry;
    ImageProps image;
  };

  // layout state
  float _x = 0, _y = 0, _w = 0, _h = 0; // layout position/size
  bool _active = false;                 // popup/menu enabled

  GuiElem(GuiElemType t) : type{t} { }
  GuiElem(GuiElemType t, std::initializer_list<GuiElem> x)
    : elems{x}, type{t} { }
};

struct gx::GuiTheme
{
  const Font* baseFont = nullptr;

  uint32_t colorBackground = packRGBA8(.2f,.2f,.2f,1.0f);
  uint32_t colorEdge = 0;

  uint32_t colorText = packRGBA8(1.0f,1.0f,1.0f,1.0f);
  uint32_t colorTextHover = packRGBA8(1.0f,1.0f,1.0f,1.0f);
  uint32_t colorTextPressed = packRGBA8(1.0f,1.0f,1.0f,1.0f);
  uint32_t colorTextHeldOnly = packRGBA8(1.0f,1.0f,1.0f,1.0f);

  uint32_t colorButton = packRGBA8(.4f,.4f,.4f,1.0f);
  uint32_t colorButtonHover = packRGBA8(.8f,.4f,.4f,1.0f);
  uint32_t colorButtonPressed = packRGBA8(.8f,.8f,.8f,1.0f);
  uint32_t colorButtonHeldOnly = packRGBA8(.6f,.6f,.6f,1.0f);

  uint32_t colorMenu = 0;
  uint32_t colorMenuHover = packRGBA8(.8f,.4f,.4f,1.0f);
  uint32_t colorMenuSelect = packRGBA8(.6f,.6f,.6f,1.0f);
  uint32_t colorMenuItem = packRGBA8(0.0f,0.0f,0.0f,1.0f);
  uint32_t colorMenuItemSelect = packRGBA8(.8f,.8f,.8f,1.0f);
  uint32_t colorEntry = packRGBA8(0.0f,0.0f,.2f,1.0f);
  uint32_t colorEntryFocus = packRGBA8(.1f,.1f,.3f,1.0f);
  uint32_t colorCursor = packRGBA8(1.0f,1.0f,.6f,1.0f);

  uint32_t cursorBlinkTime = 400000; // 1 sec
  uint16_t cursorWidth = 3;
  uint16_t spacing = 2;
  uint16_t border = 4;
  uint16_t entryLeftMargin = 4;
  uint16_t entryRightMargin = 4;
  uint16_t entryTopMargin = 2;
  uint16_t entryBottomMargin = 2;
};


class gx::Gui
{
 public:
  Gui(const GuiElem& rootElem);
  Gui(GuiElem&& rootElem);

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

  bool setText(int id, std::string_view text);

 private:
  GuiElem _rootElem;
  GuiTheme _theme;
  DrawListMap _dlm;
  Vec2 _pt = {};
  int _hoverID = 0;
  int _heldID = 0;
  int _pressedID = 0;
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
  void setFocusID(int id);
  void init(GuiElem& def);
  void calcSize(GuiElem& def);
  void calcPos(GuiElem& def, float base_x, float base_y);
  void drawElem(DrawContext& dc, DrawContext& dc2, const TextFormatting& tf,
                const GuiElem& def, ButtonState bstate, bool popup) const;
  void drawRec(DrawContext& dc, const GuiElem& def, uint32_t col) const;

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
    return GuiElem{GUI_HFRAME, {elems...}};
  }

  template<typename... Elems>
  inline GuiElem guiHFrame(AlignEnum align, const Elems&... elems)
  {
    GuiElem e{GUI_HFRAME, {elems...}};
    e.align = align;
    return e;
  }

  // VFrame
  template<typename... Elems>
  inline GuiElem guiVFrame(const Elems&... elems)
  {
    return GuiElem{GUI_VFRAME, {elems...}};
  }

  template<typename... Elems>
  inline GuiElem guiVFrame(AlignEnum align, const Elems&... elems)
  {
    GuiElem e{GUI_VFRAME, {elems...}};
    e.align = align;
    return e;
  }

  // Label
  inline GuiElem guiLabel(std::string_view text)
  {
    GuiElem e{GUI_LABEL};
    e.text = text;
    return e;
  }

  inline GuiElem guiLabel(AlignEnum align, std::string_view text)
  {
    GuiElem e{GUI_LABEL};
    e.text = text;
    e.align = align;
    return e;
  }

  // HLine
  inline GuiElem guiHLine()
  {
    GuiElem e{GUI_HLINE};
    e.align = ALIGN_HCENTER;
    return e;
  }

  // VLine
  inline GuiElem guiVLine()
  {
    GuiElem e{GUI_VLINE};
    e.align = ALIGN_VCENTER;
    return e;
  }

  // Button
  // (triggered on button release)
  inline GuiElem guiButton(int eventID, const GuiElem& elem)
  {
    GuiElem e{GUI_BUTTON, {elem}};
    e.id = eventID;
    return e;
  }

  inline GuiElem guiButton(AlignEnum align, int eventID, const GuiElem& elem)
  {
    GuiElem e{GUI_BUTTON, {elem}};
    e.align = align;
    e.id = eventID;
    return e;
  }

  inline GuiElem guiButton(std::string_view text, int eventID)
  {
    GuiElem e{GUI_BUTTON, {guiLabel(text)}};
    e.id = eventID;
    return e;
  }

  inline GuiElem guiButton(AlignEnum align, std::string_view text, int eventID)
  {
    GuiElem e{GUI_BUTTON, {guiLabel(text)}};
    e.align = align;
    e.id = eventID;
    return e;
  }

  // ButtonPress
  // (triggered on initial button press)
  inline GuiElem guiButtonPress(int eventID, const GuiElem& elem)
  {
    GuiElem e{GUI_BUTTON_PRESS, {elem}};
    e.id = eventID;
    return e;
  }

  inline GuiElem guiButtonPress(
    AlignEnum align, int eventID, const GuiElem& elem)
  {
    GuiElem e{GUI_BUTTON_PRESS, {elem}};
    e.align = align;
    e.id = eventID;
    return e;
  }

  inline GuiElem guiButtonPress(std::string_view text, int eventID)
  {
    GuiElem e{GUI_BUTTON_PRESS, {guiLabel(text)}};
    e.id = eventID;
    return e;
  }

  inline GuiElem guiButtonPress(
    AlignEnum align, std::string_view text, int eventID)
  {
    GuiElem e{GUI_BUTTON_PRESS, {guiLabel(text)}};
    e.align = align;
    e.id = eventID;
    return e;
  }

  // ButtonHold
  // (triggered repeatedly while held)
  inline GuiElem guiButtonHold(int eventID, const GuiElem& elem)
  {
    GuiElem e{GUI_BUTTON_HOLD, {elem}};
    e.id = eventID;
    return e;
  }

  inline GuiElem guiButtonHold(
    AlignEnum align, int eventID, const GuiElem& elem)
  {
    GuiElem e{GUI_BUTTON_HOLD, {elem}};
    e.align = align;
    e.id = eventID;
    return e;
  }

  inline GuiElem guiButtonHold(std::string_view text, int eventID)
  {
    GuiElem e{GUI_BUTTON_HOLD, {guiLabel(text)}};
    e.id = eventID;
    return e;
  }

  inline GuiElem guiButtonHold(
    AlignEnum align, std::string_view text, int eventID)
  {
    GuiElem e{GUI_BUTTON_HOLD, {guiLabel(text)}};
    e.align = align;
    e.id = eventID;
    return e;
  }

  // Menu
  template<typename... Elems>
  inline GuiElem guiMenu(std::string_view text, const Elems&... items)
  {
    return GuiElem{GUI_MENU, {guiLabel(text),
        GuiElem{GUI_MENU_VFRAME,{items...}}}};
  }

  inline GuiElem guiMenuItem(std::string_view text, int eventID)
  {
    GuiElem e{GUI_MENU_ITEM, {guiLabel(text)}};
    e.id = eventID;
    return e;
  }

  // Entry
  inline GuiElem guiTextEntry(float size, int maxLength, int eventID)
  {
    GuiElem e{GUI_ENTRY};
    e.entry.size = size;
    e.entry.maxLength = maxLength;
    e.entry.type = ENTRY_TEXT;
    e.id = eventID;
    return e;
  }

  inline GuiElem guiCardinalEntry(float size, int maxLength, int eventID)
  {
    GuiElem e{GUI_ENTRY};
    e.entry.size = size;
    e.entry.maxLength = maxLength;
    e.entry.type = ENTRY_CARDINAL;
    e.id = eventID;
    return e;
  }

  inline GuiElem guiIntegerEntry(float size, int maxLength, int eventID)
  {
    GuiElem e{GUI_ENTRY};
    e.entry.size = size;
    e.entry.maxLength = maxLength;
    e.entry.type = ENTRY_INTEGER;
    e.id = eventID;
    return e;
  }

  inline GuiElem guiFloatEntry(float size, int maxLength, int eventID)
  {
    GuiElem e{GUI_ENTRY};
    e.entry.size = size;
    e.entry.maxLength = maxLength;
    e.entry.type = ENTRY_FLOAT;
    e.id = eventID;
    return e;
  }

  inline GuiElem guiPasswordEntry(float size, int maxLength, int eventID)
  {
    GuiElem e{GUI_ENTRY};
    e.entry.size = size;
    e.entry.maxLength = maxLength;
    e.entry.type = ENTRY_PASSWORD;
    e.id = eventID;
    return e;
  }

  // Image
  inline GuiElem guiImage(float w, float h, TextureID tid, Vec2 t0, Vec2 t1)
  {
    GuiElem e{GUI_IMAGE};
    e.image.width = w;
    e.image.height = h;
    e.image.texId = tid;
    e.image.texCoord0 = t0;
    e.image.texCoord1 = t1;
    return e;
  }
}
