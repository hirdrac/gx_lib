//
// gx/Gui.hh
// Copyright (C) 2020 Richard Bradley
//
// Graphical user interface rendering & event handling
//

#pragma once
#include "DrawList.hh"
#include "Color.hh"
#include <string_view>
#include <vector>


namespace gx {
  class Window;
  class Font;
  class Gui;
  struct GuiElem;
  struct GuiTheme;

  enum GuiElemType {
    GUI_HFRAME, GUI_VFRAME, GUI_LABEL, GUI_HLINE, GUI_VLINE, GUI_BUTTON
  };

  enum {
    GUIFLAG_DISABLE      = 1<<0,
    GUIFLAG_EQUAL_WIDTH  = 1<<1,
    GUIFLAG_EQUAL_HEIGHT = 1<<2,
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
  std::vector<GuiElem> elems;  // child elements
  std::string text;  // label text
  AlignEnum align = ALIGN_UNSPECIFIED;
  int flags = 0;
  int eventID = 0;
  GuiElemType type;

  // layout result
  float _x, _y, _w, _h;

  GuiElem(GuiElemType t) : type(t) { }
};

struct gx::GuiTheme
{
  const Font* baseFont = nullptr;
  uint32_t colorButtonNormal = packRGBA8(.4,.4,.4,1);
  uint32_t colorButtonHover = packRGBA8(.8,.4,.4,1);
  uint32_t colorButtonPressed = packRGBA8(.8,.8,.8,1);
  uint32_t colorButtonHeldOnly = packRGBA8(.6,.6,.6,1);
  uint32_t colorBackground = packRGBA8(.2,.2,.2,1);
  uint32_t colorText = packRGBA8(1,1,1,1);
  uint32_t colorFrame = 0; //packRGBA8(1,1,1,1);
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

  [[nodiscard]] int buttonHover() const { return _buttonHoverID; }
    // id of button mouse is over
  [[nodiscard]] int buttonHeld() const { return _buttonHeldID; }
    // id of button last pressed, set until button is released
  [[nodiscard]] int buttonPressed() const { return _buttonPressedID; }
    // id of button pressed, cleared on next update call
  [[nodiscard]] int buttonReleased() const { return _buttonReleasedID; }
    // id of button released, cleared on next update call
    // (was previously pressed, button released while still over same button)
  [[nodiscard]] bool needRedraw() const { return _needRedraw; }
    // true if GUI needs to be redrawn

 private:
  GuiElem _rootElem;
  GuiTheme _theme;
  DrawList _dl;
  Vec2 _pt = {};
  AlignEnum _align = ALIGN_TOP_LEFT;
  int _buttonHoverID = 0;
  int _buttonHeldID = 0;
  int _buttonPressedID = 0;
  int _lastPressedID = 0;
  int _buttonReleasedID = 0;
  bool _needSize = true;
  bool _needPos = true;
  bool _needRender = true;
  bool _needRedraw = true;

  void calcSize(GuiElem& def);
  void calcPos(GuiElem& def, float base_x, float base_y);
  void drawElem(GuiElem& def, ButtonState bstate);
  GuiElem* findEventElem(float x, float y);
  GuiElem* findElem(int eventID);
  void drawRec(const GuiElem& def, uint32_t col);
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
    e.eventID = eventID;
    return e;
  }

  inline GuiElem guiButton(AlignEnum align, int eventID, const GuiElem& elem)
  {
    GuiElem e(GUI_BUTTON);
    e.elems.push_back(elem);
    e.align = align;
    e.eventID = eventID;
    return e;
  }

  inline GuiElem guiButton(std::string_view text, int eventID)
  {
    GuiElem e(GUI_BUTTON);
    e.elems.push_back(gx::guiLabel(text));
    e.eventID = eventID;
    return e;
  }

  inline GuiElem guiButton(AlignEnum align, std::string_view text, int eventID)
  {
    GuiElem e(GUI_BUTTON);
    e.elems.push_back(gx::guiLabel(text));
    e.align = align;
    e.eventID = eventID;
    return e;
  }
}
