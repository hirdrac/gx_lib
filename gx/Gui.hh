//
// gx/Gui.hh
// Copyright (C) 2021 Richard Bradley
//
// Graphical user interface rendering & event handling
//

// TODO - sub menus
// TODO - menu item key short-cuts
// TODO - list select (combo box)
// TODO - control allowed char types for entry (numeric only, etc.)

#pragma once
#include "DrawList.hh"
#include "Align.hh"
#include "Color.hh"
#include "Types.hh"
#include <string_view>
#include <vector>


namespace gx {
  class Window;
  class DrawContext;
  class Font;
  class Gui;
  struct GuiElem;
  struct GuiTheme;

  enum GuiElemType {
    GUI_NULL = 0,
    GUI_HFRAME, GUI_VFRAME, GUI_LABEL, GUI_HLINE, GUI_VLINE, GUI_BUTTON,
    GUI_MENU, GUI_MENU_HFRAME, GUI_MENU_VFRAME, GUI_MENU_ITEM,
    GUI_ENTRY, GUI_IMAGE
  };

  enum ButtonState {
    BSTATE_NONE = 0,  // no button
    BSTATE_NORMAL,    // normal button
    BSTATE_HOVER,     // pointer over button
    BSTATE_PRESSED,   // pointer over button with mouse left button pressed
    BSTATE_HELD_ONLY  // previously pressed & mouse left button still held
                      //   but pointer not over button
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

  GuiElem(GuiElemType t) : type(t) { }
};

struct gx::GuiTheme
{
  const Font* baseFont = nullptr;
  uint32_t colorBackground = packRGBA8(.2,.2,.2,1);
  uint32_t colorText = packRGBA8(1,1,1,1);
  uint32_t colorEdge = 0; //packRGBA8(1,1,1,1);
  uint32_t colorButtonNormal = packRGBA8(.4,.4,.4,1);
  uint32_t colorButtonHover = packRGBA8(.8,.4,.4,1);
  uint32_t colorButtonPressed = packRGBA8(.8,.8,.8,1);
  uint32_t colorButtonHeldOnly = packRGBA8(.6,.6,.6,1);
  uint32_t colorMenuNormal = 0;
  uint32_t colorMenuHover = packRGBA8(.8,.4,.4,1);
  uint32_t colorMenuSelect = packRGBA8(.6,.6,.6,1);
  uint32_t colorMenuItem = packRGBA8(0,0,0,1);
  uint32_t colorMenuItemSelect = packRGBA8(.8,.8,.8,1);
  uint32_t colorEntry = packRGBA8(0,0,.2,1);
  uint32_t colorEntryFocus = packRGBA8(.1,.1,.3,1);
  uint32_t colorCursor = packRGBA8(1,1,.6,1);
  uint32_t cursorBlinkTime = 400000; // 1 sec
  uint32_t cursorWidth = 3;
  int16_t spacing = 2;
  int16_t border = 4;
};


class gx::Gui
{
 public:
  Gui(const GuiElem& rootElem);
  Gui(GuiElem&& rootElem);

  void layout(const GuiTheme& theme, float x, float y, AlignEnum align);
  void update(Window& win);

  [[nodiscard]] const DrawList& drawList() const { return _dl; }
  [[nodiscard]] float width() const { return _rootElem._w; }
  [[nodiscard]] float height() const { return _rootElem._h; }

  [[nodiscard]] int hoverID() const { return _hoverID; }
    // id of button mouse is over
  [[nodiscard]] int heldID() const { return _heldID; }
    // id of button last pressed, set until button is released
  [[nodiscard]] int pressedID() const { return _pressedID; }
    // id of button pressed, cleared on next update call
  [[nodiscard]] int releasedID() const { return _releasedID; }
    // id of button released, cleared on next update call
    // (was previously pressed, button released while still over same button)
  [[nodiscard]] int focusID() const { return _focusID; }
    // id of active text entry
  [[nodiscard]] int entryID() const { return _entryID; }
    // id of text entry with value update (triggered with tab/enter)
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
  DrawList _dl;
  Vec2 _pt = {};
  int _hoverID = 0;
  int _heldID = 0;
  int _pressedID = 0;
  int _releasedID = 0;
  int _lastPressedID = 0;
  int _focusID = 0;
  int _entryID = 0;
  GuiElemType _lastType = GUI_NULL;
  int64_t _lastCursorUpdate = 0;
  int _uniqueID = -1;
  bool _cursorState = false;
  bool _needSize = true;
  bool _needPos = true;
  bool _needRender = true;
  bool _needRedraw = true;
  bool _textChanged = false;

  void init(GuiElem& def);
  void calcSize(GuiElem& def);
  void calcPos(GuiElem& def, float base_x, float base_y);
  void deactivate(GuiElem& def);
  void drawElem(DrawContext& dc, GuiElem& def, ButtonState bstate);
  GuiElem* findElem(float x, float y);
  GuiElem* findElem(int id);
  const GuiElem* findElem(int id) const;
  GuiElem* findNextElem(int id, GuiElemType type = GUI_NULL);
  GuiElem* findPrevElem(int id, GuiElemType type = GUI_NULL);
  void drawRec(DrawContext& dc, const GuiElem& def, uint32_t col);
  void processCharEvent(Window& win);
  void setFocusID(int id);
};


namespace gx {
  // **** GuiElem functions ****

  // HFrame
  template<typename... Elems>
  inline GuiElem guiHFrame(const Elems&... elems)
  {
    GuiElem e(GUI_HFRAME);
    e.elems = {elems...};
    return e;
  }

  template<typename... Elems>
  inline GuiElem guiHFrame(AlignEnum align, const Elems&... elems)
  {
    GuiElem e(GUI_HFRAME);
    e.elems = {elems...};
    e.align = align;
    return e;
  }

  // VFrame
  template<typename... Elems>
  inline GuiElem guiVFrame(const Elems&... elems)
  {
    GuiElem e(GUI_VFRAME);
    e.elems = {elems...};
    return e;
  }

  template<typename... Elems>
  inline GuiElem guiVFrame(AlignEnum align, const Elems&... elems)
  {
    GuiElem e(GUI_VFRAME);
    e.elems = {elems...};
    e.align = align;
    return e;
  }

  // Label
  inline GuiElem guiLabel(std::string_view text)
  {
    GuiElem e(GUI_LABEL);
    e.text = text;
    return e;
  }

  inline GuiElem guiLabel(AlignEnum align, std::string_view text)
  {
    GuiElem e(GUI_LABEL);
    e.text = text;
    e.align = align;
    return e;
  }

  // HLine
  inline GuiElem guiHLine()
  {
    GuiElem e(GUI_HLINE);
    e.align = ALIGN_HCENTER;
    return e;
  }

  // VLine
  inline GuiElem guiVLine()
  {
    GuiElem e(GUI_VLINE);
    e.align = ALIGN_VCENTER;
    return e;
  }

  // Button
  inline GuiElem guiButton(int eventID, const GuiElem& elem)
  {
    GuiElem e(GUI_BUTTON);
    e.elems.push_back(elem);
    e.id = eventID;
    return e;
  }

  inline GuiElem guiButton(AlignEnum align, int eventID, const GuiElem& elem)
  {
    GuiElem e(GUI_BUTTON);
    e.elems.push_back(elem);
    e.align = align;
    e.id = eventID;
    return e;
  }

  inline GuiElem guiButton(std::string_view text, int eventID)
  {
    GuiElem e(GUI_BUTTON);
    e.elems.push_back(gx::guiLabel(text));
    e.id = eventID;
    return e;
  }

  inline GuiElem guiButton(AlignEnum align, std::string_view text, int eventID)
  {
    GuiElem e(GUI_BUTTON);
    e.elems.push_back(gx::guiLabel(text));
    e.align = align;
    e.id = eventID;
    return e;
  }

  // Menu
  template<typename... Elems>
  inline GuiElem guiMenu(std::string_view text, const Elems&... items)
  {
    GuiElem e(GUI_MENU);
    e.elems.push_back(gx::guiLabel(text));
    GuiElem& frame = e.elems.emplace_back(GUI_MENU_VFRAME);
    frame.elems = {items...};
    return e;
  }

  inline GuiElem guiMenuItem(std::string_view text, int eventID)
  {
    GuiElem e(GUI_MENU_ITEM);
    e.elems.push_back(gx::guiLabel(text));
    e.id = eventID;
    return e;
  }

  // Entry
  inline GuiElem guiEntry(float size, int maxLength, int eventID)
  {
    GuiElem e(GUI_ENTRY);
    e.entry.size = size;
    e.entry.maxLength = maxLength;
    e.id = eventID;
    return e;
  }

  // Image
  inline GuiElem guiImage(float w, float h, TextureID tid, Vec2 t0, Vec2 t1)
  {
    GuiElem e(GUI_IMAGE);
    e.image.width = w;
    e.image.height = h;
    e.image.texId = tid;
    e.image.texCoord0 = t0;
    e.image.texCoord1 = t1;
    return e;
  }
}
