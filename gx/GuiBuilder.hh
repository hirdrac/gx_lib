//
// gx/GuiBuilder.hh
// Copyright (C) 2023 Richard Bradley
//
// Functions for creating GuiElem objects for use with Gui class
//

#pragma once
#include "GuiElem.hh"
#include "Align.hh"
#include "Types.hh"
#include <string_view>


namespace gx {
  // **** GuiElem functions ****

  // HFrame
  template<typename... Elems>
  [[nodiscard]] inline GuiElem guiHFrame(const Elems&... elems) {
    return {GUI_HFRAME, ALIGN_TOP_LEFT, 0, {elems...}};
  }

  template<typename... Elems>
  [[nodiscard]] inline GuiElem guiHFrame(
    AlignEnum align, const Elems&... elems) {
    return {GUI_HFRAME, align, 0, {elems...}};
  }

  // VFrame
  template<typename... Elems>
  [[nodiscard]] inline GuiElem guiVFrame(const Elems&... elems) {
    return {GUI_VFRAME, ALIGN_TOP_LEFT, 0, {elems...}};
  }

  template<typename... Elems>
  [[nodiscard]] inline GuiElem guiVFrame(
    AlignEnum align, const Elems&... elems) {
    return {GUI_VFRAME, align, 0, {elems...}};
  }

  // Margin Set
  [[nodiscard]] inline GuiElem&& guiMargin(
    GuiElem&& elem, int16_t left, int16_t top, int16_t right, int16_t bottom) {
    elem.l_margin = left;
    elem.t_margin = top;
    elem.r_margin = right;
    elem.b_margin = bottom;
    return std::move(elem);
  }

  // Label
  [[nodiscard]] inline GuiElem guiLabel(
    std::string_view text, float minLength = 0, int minLines = 0)
  {
    GuiElem e{GUI_LABEL, ALIGN_TOP_LEFT, 0};
    GuiElem::LabelProps label;
    label.text = text;
    label.minLength = minLength;
    label.minLines = minLines;
    e.props = label;
    return e;
  }

  [[nodiscard]] inline GuiElem guiLabel(
    AlignEnum align, std::string_view text,
    float minLength = 0, int minLines = 0)
  {
    GuiElem e{GUI_LABEL, align, 0};
    GuiElem::LabelProps label;
    label.text = text;
    label.minLength = minLength;
    label.minLines = minLines;
    e.props = label;
    return e;
  }

  [[nodiscard]] inline GuiElem guiLabel(
    EventID id, std::string_view text, float minLength = 0, int minLines = 0)
  {
    GuiElem e{GUI_LABEL, ALIGN_TOP_LEFT, id};
    GuiElem::LabelProps label;
    label.text = text;
    label.minLength = minLength;
    label.minLines = minLines;
    e.props = label;
    return e;
  }

  [[nodiscard]] inline GuiElem guiLabel(
    EventID id, AlignEnum align, std::string_view text,
    float minLength = 0, int minLines = 0)
  {
    GuiElem e{GUI_LABEL, align, id};
    GuiElem::LabelProps label;
    label.text = text;
    label.minLength = minLength;
    label.minLines = minLines;
    e.props = label;
    return e;
  }

  // VLabel
  [[nodiscard]] inline GuiElem guiVLabel(
    std::string_view text, float minLength = 0, int minLines = 0)
  {
    GuiElem e{GUI_VLABEL, ALIGN_BOTTOM_LEFT, 0};
    GuiElem::LabelProps label;
    label.text = text;
    label.minLength = minLength;
    label.minLines = minLines;
    e.props = label;
    return e;
  }

  [[nodiscard]] inline GuiElem guiVLabel(
    AlignEnum align, std::string_view text,
    float minLength = 0, int minLines = 0)
  {
    GuiElem e{GUI_VLABEL, align, 0};
    GuiElem::LabelProps label;
    label.text = text;
    label.minLength = minLength;
    label.minLines = minLines;
    e.props = label;
    return e;
  }

  [[nodiscard]] inline GuiElem guiVLabel(
    EventID id, std::string_view text, float minLength = 0, int minLines = 0)
  {
    GuiElem e{GUI_VLABEL, ALIGN_BOTTOM_LEFT, id};
    GuiElem::LabelProps label;
    label.text = text;
    label.minLength = minLength;
    label.minLines = minLines;
    e.props = label;
    return e;
  }

  [[nodiscard]] inline GuiElem guiVLabel(
    EventID id, AlignEnum align, std::string_view text,
    float minLength = 0, int minLines = 0)
  {
    GuiElem e{GUI_VLABEL, align, id};
    GuiElem::LabelProps label;
    label.text = text;
    label.minLength = minLength;
    label.minLines = minLines;
    e.props = label;
    return e;
  }

  // HLine
  [[nodiscard]] inline GuiElem guiHLine() {
    return {GUI_HLINE, ALIGN_HJUSTIFY, 0};
  }

  // VLine
  [[nodiscard]] inline GuiElem guiVLine() {
    return {GUI_VLINE, ALIGN_VJUSTIFY, 0};
  }

  // Button
  // (triggered on button release)
  [[nodiscard]] inline GuiElem guiButton(EventID id, const GuiElem& elem)
  {
    GuiElem e{GUI_BUTTON, ALIGN_TOP_LEFT, id, {elem}};
    e.props = GuiElem::ButtonProps{};
    return e;
  }

  [[nodiscard]] inline GuiElem guiButton(
    EventID id, AlignEnum align, const GuiElem& elem)
  {
    GuiElem e{GUI_BUTTON, align, id, {elem}};
    e.props = GuiElem::ButtonProps{};
    return e;
  }

  [[nodiscard]] inline GuiElem guiButton(EventID id, std::string_view text)
  {
    GuiElem e{GUI_BUTTON, ALIGN_TOP_LEFT, id, {guiLabel(ALIGN_CENTER, text)}};
    e.props = GuiElem::ButtonProps{};
    return e;
  }

  [[nodiscard]] inline GuiElem guiButton(
    EventID id, AlignEnum align, std::string_view text)
  {
    GuiElem e{GUI_BUTTON, align, id, {guiLabel(ALIGN_CENTER, text)}};
    e.props = GuiElem::ButtonProps{};
    return e;
  }

  // ButtonPress
  // (triggered on initial button press, holding repeats if repeat_delay >= 0)
  [[nodiscard]] inline GuiElem guiButtonPress(
    EventID id, int64_t repeat_delay, const GuiElem& elem)
  {
    GuiElem e{GUI_BUTTON_PRESS, ALIGN_TOP_LEFT, id, {elem}};
    e.props = GuiElem::ButtonProps{.repeatDelay = repeat_delay};
    return e;
  }

  [[nodiscard]] inline GuiElem guiButtonPress(
    EventID id, AlignEnum align, int64_t repeat_delay, const GuiElem& elem)
  {
    GuiElem e{GUI_BUTTON_PRESS, align, id, {elem}};
    e.props = GuiElem::ButtonProps{.repeatDelay = repeat_delay};
    return e;
  }

  [[nodiscard]] inline GuiElem guiButtonPress(
    EventID id, int64_t repeat_delay, std::string_view text)
  {
    GuiElem e{GUI_BUTTON_PRESS, ALIGN_TOP_LEFT, id,
              {guiLabel(ALIGN_CENTER, text)}};
    e.props = GuiElem::ButtonProps{.repeatDelay = repeat_delay};
    return e;
  }

  [[nodiscard]] inline GuiElem guiButtonPress(
    EventID id, AlignEnum align, int64_t repeat_delay, std::string_view text)
  {
    GuiElem e{GUI_BUTTON_PRESS, align, id, {guiLabel(ALIGN_CENTER, text)}};
    e.props = GuiElem::ButtonProps{.repeatDelay = repeat_delay};
    return e;
  }

  // Checkbox
  [[nodiscard]] inline GuiElem guiCheckbox(
    EventID id, bool set, const GuiElem& label)
  {
    GuiElem e{GUI_CHECKBOX, ALIGN_LEFT, id, {label}};
    e.props = GuiElem::CheckboxProps{.set = set};
    return e;
  }

  [[nodiscard]] inline GuiElem guiCheckbox(
    EventID id, AlignEnum align, bool set, const GuiElem& label)
  {
    GuiElem e{GUI_CHECKBOX, align, id, {label}};
    e.props = GuiElem::CheckboxProps{.set = set};
    return e;
  }

  [[nodiscard]] inline GuiElem guiCheckbox(
    EventID id, bool set, std::string_view label)
  {
    GuiElem e{GUI_CHECKBOX, ALIGN_TOP_LEFT, id, {guiLabel(ALIGN_LEFT, label)}};
    e.props = GuiElem::CheckboxProps{.set = set};
    return e;
  }

  [[nodiscard]] inline GuiElem guiCheckbox(
    EventID id, AlignEnum align, bool set, std::string_view label)
  {
    GuiElem e{GUI_CHECKBOX, align, id, {guiLabel(ALIGN_LEFT, label)}};
    e.props = GuiElem::CheckboxProps{.set = set};
    return e;
  }

  // Menu
  template<typename... Elems>
  [[nodiscard]] inline GuiElem guiMenu(
    EventID id, std::string_view text, const Elems&... items)
  {
    return {GUI_MENU, ALIGN_TOP_LEFT, id,
            {guiLabel(ALIGN_CENTER, text),
             GuiElem{GUI_POPUP, ALIGN_TOP_LEFT, 0, {guiVFrame(items...)}}}};
  }

  [[nodiscard]] inline GuiElem guiMenuItem(int no, std::string_view text)
  {
    GuiElem e{GUI_MENU_ITEM, ALIGN_JUSTIFY, 0,
              {guiLabel(ALIGN_CENTER_LEFT, text)}};
    e.props = GuiElem::ItemProps{.no = no};
    return e;
  }

  template<typename... Elems>
  [[nodiscard]] inline GuiElem guiSubMenu(
    std::string_view text, const Elems&... items)
  {
    return {GUI_SUBMENU, ALIGN_JUSTIFY, 0,
            {guiLabel(ALIGN_CENTER_LEFT, text),
             GuiElem{GUI_POPUP, ALIGN_TOP_LEFT, 0, {guiVFrame(items...)}}}};
  }

  // List Select
  template<typename... Elems>
  [[nodiscard]] inline GuiElem guiListSelect(EventID id, const Elems&... items)
  {
    GuiElem e{GUI_LISTSELECT, ALIGN_TOP_LEFT, id,
              {guiLabel(""), // copy of label from selected item
               GuiElem{GUI_POPUP, ALIGN_TOP_LEFT, 0, {guiVFrame(items...)}}}};
    e.props = GuiElem::ItemProps{};
    return e;
  }

  template<typename... Elems>
  [[nodiscard]] inline GuiElem guiListSelect(
    EventID id, AlignEnum align, const Elems&... items)
  {
    GuiElem e{GUI_LISTSELECT, align, id,
              {guiLabel(""), // copy of label from selected item
               GuiElem{GUI_POPUP, ALIGN_TOP_LEFT, 0, {guiVFrame(items...)}}}};
    e.props = GuiElem::ItemProps{};
    return e;
  }

  [[nodiscard]] inline GuiElem guiListSelectItem(int no, std::string_view text)
  {
    GuiElem e{GUI_LISTSELECT_ITEM, ALIGN_JUSTIFY, 0,
              {guiLabel(ALIGN_CENTER_LEFT, text)}};
    e.props = GuiElem::ItemProps{.no = no};
    return e;
  }

  // Entry
  [[nodiscard]] inline GuiElem guiEntry(
    EventID id, AlignEnum align, EntryType type, float size,
    uint32_t maxLen, AlignEnum textAlign)
  {
    GuiElem e{GUI_ENTRY, align, id};
    GuiElem::EntryProps entry;
    if (type == ENTRY_CARDINAL || type == ENTRY_INTEGER
        || type == ENTRY_FLOAT) { entry.text = "0"; }
    entry.size = size;
    entry.maxLength = maxLen;
    entry.type = type;
    entry.align = textAlign;
    e.props = entry;
    return e;
  }

  [[nodiscard]] inline GuiElem guiTextEntry(
    EventID id, float size, uint32_t maxLen, AlignEnum textAlign = ALIGN_LEFT) {
    return guiEntry(id, ALIGN_TOP_LEFT, ENTRY_TEXT, size, maxLen, textAlign);
  }

  [[nodiscard]] inline GuiElem guiTextEntry(
    EventID id, AlignEnum align, float size, uint32_t maxLen,
    AlignEnum textAlign = ALIGN_LEFT) {
    return guiEntry(id, align, ENTRY_TEXT, size, maxLen, textAlign);
  }

  [[nodiscard]] inline GuiElem guiCardinalEntry(
    EventID id, float size, uint32_t maxLen, AlignEnum textAlign = ALIGN_LEFT) {
    return guiEntry(id, ALIGN_TOP_LEFT, ENTRY_CARDINAL, size, maxLen, textAlign);
  }

  [[nodiscard]] inline GuiElem guiCardinalEntry(
    EventID id, AlignEnum align, float size, uint32_t maxLen,
    AlignEnum textAlign = ALIGN_LEFT) {
    return guiEntry(id, align, ENTRY_CARDINAL, size, maxLen, textAlign);
  }

  [[nodiscard]] inline GuiElem guiIntegerEntry(
    EventID id, float size, uint32_t maxLen, AlignEnum textAlign = ALIGN_LEFT) {
    return guiEntry(id, ALIGN_TOP_LEFT, ENTRY_INTEGER, size, maxLen, textAlign);
  }

  [[nodiscard]] inline GuiElem guiIntegerEntry(
    EventID id, AlignEnum align, float size, uint32_t maxLen,
    AlignEnum textAlign = ALIGN_LEFT) {
    return guiEntry(id, align, ENTRY_INTEGER, size, maxLen, textAlign);
  }

  [[nodiscard]] inline GuiElem guiFloatEntry(
    EventID id, float size, uint32_t maxLen, AlignEnum textAlign = ALIGN_LEFT) {
    return guiEntry(id, ALIGN_TOP_LEFT, ENTRY_FLOAT, size, maxLen, textAlign);
  }

  [[nodiscard]] inline GuiElem guiFloatEntry(
    EventID id, AlignEnum align, float size, uint32_t maxLen,
    AlignEnum textAlign = ALIGN_LEFT) {
    return guiEntry(id, align, ENTRY_FLOAT, size, maxLen, textAlign);
  }

  [[nodiscard]] inline GuiElem guiPasswordEntry(
    EventID id, float size, uint32_t maxLen, AlignEnum textAlign = ALIGN_LEFT) {
    return guiEntry(id, ALIGN_TOP_LEFT, ENTRY_PASSWORD, size, maxLen, textAlign);
  }

  [[nodiscard]] inline GuiElem guiPasswordEntry(
    EventID id, AlignEnum align, float size, uint32_t maxLen,
    AlignEnum textAlign = ALIGN_LEFT) {
    return guiEntry(id, align, ENTRY_PASSWORD, size, maxLen, textAlign);
  }

  // Image
  [[nodiscard]] inline GuiElem guiImage(
    float w, float h, TextureID tid, Vec2 t0, Vec2 t1)
  {
    GuiElem e{GUI_IMAGE, ALIGN_TOP_LEFT, 0};
    GuiElem::ImageProps image;
    image.width = w;
    image.height = h;
    image.texId = tid;
    image.texCoord0 = t0;
    image.texCoord1 = t1;
    e.props = image;
    return e;
  }

  // TitleBar
  [[nodiscard]] inline GuiElem guiTitleBar() {
    return {GUI_TITLEBAR, ALIGN_HJUSTIFY, 0};
  }

  [[nodiscard]] inline GuiElem guiTitleBar(std::string_view text) {
    return {GUI_TITLEBAR, ALIGN_HJUSTIFY, 0, {guiLabel(ALIGN_CENTER, text)}};
  }

  // VTitleBar
  [[nodiscard]] inline GuiElem guiVTitleBar() {
    return {GUI_TITLEBAR, ALIGN_VJUSTIFY, 0};
  }

  [[nodiscard]] inline GuiElem guiVTitleBar(std::string_view text) {
    return {GUI_TITLEBAR, ALIGN_VJUSTIFY, 0, {guiVLabel(ALIGN_CENTER, text)}};
  }
}
