//
// gx/GuiBuilder.hh
// Copyright (C) 2022 Richard Bradley
//
// Functions for creating GuiElem objects for use with Gui class
//

#pragma once
#include "Gui.hh"
#include "Align.hh"
#include "Types.hh"
#include <string_view>


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
    return {GUI_LABEL, ALIGN_TOP_LEFT, 0, text};
  }

  inline GuiElem guiLabel(AlignEnum align, std::string_view text)
  {
    return {GUI_LABEL, align, 0, text};
  }

  inline GuiElem guiLabel(EventID id, std::string_view text)
  {
    return {GUI_LABEL, ALIGN_TOP_LEFT, id, text};
  }

  inline GuiElem guiLabel(EventID id, AlignEnum align, std::string_view text)
  {
    return {GUI_LABEL, align, id, text};
  }

  // VLabel
  inline GuiElem guiVLabel(std::string_view text)
  {
    return {GUI_VLABEL, ALIGN_BOTTOM_LEFT, 0, text};
  }

  inline GuiElem guiVLabel(AlignEnum align, std::string_view text)
  {
    return {GUI_VLABEL, align, 0, text};
  }

  inline GuiElem guiVLabel(EventID id, std::string_view text)
  {
    return {GUI_VLABEL, ALIGN_BOTTOM_LEFT, id, text};
  }

  inline GuiElem guiVLabel(EventID id, AlignEnum align, std::string_view text)
  {
    return {GUI_VLABEL, align, id, text};
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
    GuiElem e{GUI_BUTTON, ALIGN_TOP_LEFT, id, {elem}};
    e.button.repeatDelay = -1; // disabled
    e.button.action = ACTION_NONE;
    e.button.targetID = 0;
    return e;
  }

  inline GuiElem guiButton(EventID id, AlignEnum align, const GuiElem& elem)
  {
    GuiElem e{GUI_BUTTON, align, id, {elem}};
    e.button.repeatDelay = -1; // disabled
    e.button.action = ACTION_NONE;
    e.button.targetID = 0;
    return e;
  }

  inline GuiElem guiButton(EventID id, std::string_view text)
  {
    GuiElem e{GUI_BUTTON, ALIGN_TOP_LEFT, id, {guiLabel(ALIGN_CENTER, text)}};
    e.button.repeatDelay = -1; // disabled
    e.button.action = ACTION_NONE;
    e.button.targetID = 0;
    return e;
  }

  inline GuiElem guiButton(EventID id, AlignEnum align, std::string_view text)
  {
    GuiElem e{GUI_BUTTON, align, id, {guiLabel(ALIGN_CENTER, text)}};
    e.button.repeatDelay = -1; // disabled
    e.button.action = ACTION_NONE;
    e.button.targetID = 0;
    return e;
  }

  // ButtonPress
  // (triggered on initial button press)
  inline GuiElem guiButtonPress(EventID id, const GuiElem& elem)
  {
    GuiElem e{GUI_BUTTON_PRESS, ALIGN_TOP_LEFT, id, {elem}};
    e.button.repeatDelay = -1; // disabled
    e.button.action = ACTION_NONE;
    e.button.targetID = 0;
    return e;
  }

  inline GuiElem guiButtonPress(
    EventID id, AlignEnum align, const GuiElem& elem)
  {
    GuiElem e{GUI_BUTTON_PRESS, align, id, {elem}};
    e.button.repeatDelay = -1; // disabled
    e.button.action = ACTION_NONE;
    e.button.targetID = 0;
    return e;
  }

  inline GuiElem guiButtonPress(EventID id, std::string_view text)
  {
    GuiElem e{GUI_BUTTON_PRESS, ALIGN_TOP_LEFT, id,
              {guiLabel(ALIGN_CENTER, text)}};
    e.button.repeatDelay = -1; // disabled
    e.button.action = ACTION_NONE;
    e.button.targetID = 0;
    return e;
  }

  inline GuiElem guiButtonPress(
    EventID id, AlignEnum align, std::string_view text)
  {
    GuiElem e{GUI_BUTTON_PRESS, align, id, {guiLabel(ALIGN_CENTER, text)}};
    e.button.repeatDelay = -1; // disabled
    e.button.action = ACTION_NONE;
    e.button.targetID = 0;
    return e;
  }

  // ButtonPress with repeat if held
  inline GuiElem guiButtonHold(
    EventID id, int64_t repeat_delay, const GuiElem& elem)
  {
    GuiElem e{GUI_BUTTON_PRESS, ALIGN_TOP_LEFT, id, {elem}};
    e.button.repeatDelay = repeat_delay;
    e.button.action = ACTION_NONE;
    e.button.targetID = 0;
    return e;
  }

  inline GuiElem guiButtonHold(
    EventID id, AlignEnum align, int64_t repeat_delay, const GuiElem& elem)
  {
    GuiElem e{GUI_BUTTON_PRESS, align, id, {elem}};
    e.button.repeatDelay = repeat_delay;
    e.button.action = ACTION_NONE;
    e.button.targetID = 0;
    return e;
  }

  inline GuiElem guiButtonHold(
    EventID id, int64_t repeat_delay, std::string_view text)
  {
    GuiElem e{GUI_BUTTON_PRESS, ALIGN_TOP_LEFT, id,
              {guiLabel(ALIGN_CENTER, text)}};
    e.button.repeatDelay = repeat_delay;
    e.button.action = ACTION_NONE;
    e.button.targetID = 0;
    return e;
  }

  inline GuiElem guiButtonHold(
    EventID id, AlignEnum align, int64_t repeat_delay, std::string_view text)
  {
    GuiElem e{GUI_BUTTON_PRESS, align, id, {guiLabel(ALIGN_CENTER, text)}};
    e.button.repeatDelay = repeat_delay;
    e.button.action = ACTION_NONE;
    e.button.targetID = 0;
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
             GuiElem{GUI_POPUP, ALIGN_TOP_LEFT, 0, {guiVFrame(items...)}}}};
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
             GuiElem{GUI_POPUP, ALIGN_TOP_LEFT, 0, {guiVFrame(items...)}}}};
  }

  // List Select
  template<typename... Elems>
  inline GuiElem guiListSelect(EventID id, const Elems&... items)
  {
    GuiElem e{GUI_LISTSELECT, ALIGN_TOP_LEFT, id,
              {GuiElem{},
               GuiElem{GUI_POPUP, ALIGN_TOP_LEFT, 0, {guiVFrame(items...)}}}};
    e.itemNo = 0; // unset (default to first item)
    return e;
  }

  template<typename... Elems>
  inline GuiElem guiListSelect(EventID id, AlignEnum align,
                               const Elems&... items)
  {
    GuiElem e{GUI_LISTSELECT, align, id,
              {GuiElem{},
               GuiElem{GUI_POPUP, ALIGN_TOP_LEFT, 0, {guiVFrame(items...)}}}};
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

  inline GuiElem guiTitleBar(std::string_view text)
  {
    return {GUI_TITLEBAR, ALIGN_HJUSTIFY, 0, {guiLabel(ALIGN_CENTER, text)}};
  }

  // VTitleBar
  inline GuiElem guiVTitleBar()
  {
    return {GUI_TITLEBAR, ALIGN_VJUSTIFY, 0};
  }

  inline GuiElem guiVTitleBar(std::string_view text)
  {
    return {GUI_TITLEBAR, ALIGN_VJUSTIFY, 0, {guiVLabel(ALIGN_CENTER, text)}};
  }
}
