//
// gx/GuiBuilder.hh
// Copyright (C) 2026 Richard Bradley
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
  template<class... Elems>
  [[nodiscard]] inline GuiElem guiHFrame(const Elems&... elems) {
    return {GUI_HFRAME, Align::top_left, 0, {elems...}};
  }

  template<class... Elems>
  [[nodiscard]] inline GuiElem guiHFrame(Align align, const Elems&... elems) {
    return {GUI_HFRAME, align, 0, {elems...}};
  }

  // VFrame
  template<class... Elems>
  [[nodiscard]] inline GuiElem guiVFrame(const Elems&... elems) {
    return {GUI_VFRAME, Align::top_left, 0, {elems...}};
  }

  template<class... Elems>
  [[nodiscard]] inline GuiElem guiVFrame(Align align, const Elems&... elems) {
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
    GuiElem e{GUI_LABEL, Align::top_left, 0};
    GuiElem::LabelProps label;
    label.text = text;
    label.minLength = minLength;
    label.minLines = minLines;
    e.props = label;
    return e;
  }

  [[nodiscard]] inline GuiElem guiLabel(Align align, std::string_view text,
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
    GuiElem e{GUI_LABEL, Align::top_left, id};
    GuiElem::LabelProps label;
    label.text = text;
    label.minLength = minLength;
    label.minLines = minLines;
    e.props = label;
    return e;
  }

  [[nodiscard]] inline GuiElem guiLabel(
    EventID id, Align align, std::string_view text,
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
    GuiElem e{GUI_VLABEL, Align::bottom_left, 0};
    GuiElem::LabelProps label;
    label.text = text;
    label.minLength = minLength;
    label.minLines = minLines;
    e.props = label;
    return e;
  }

  [[nodiscard]] inline GuiElem guiVLabel(
    Align align, std::string_view text,
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
    GuiElem e{GUI_VLABEL, Align::bottom_left, id};
    GuiElem::LabelProps label;
    label.text = text;
    label.minLength = minLength;
    label.minLines = minLines;
    e.props = label;
    return e;
  }

  [[nodiscard]] inline GuiElem guiVLabel(
    EventID id, Align align, std::string_view text,
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
    return {GUI_HLINE, Align::hjustify, 0};
  }

  // VLine
  [[nodiscard]] inline GuiElem guiVLine() {
    return {GUI_VLINE, Align::vjustify, 0};
  }

  // Button
  // (triggered on button release)
  [[nodiscard]] inline GuiElem guiButton(EventID id, const GuiElem& elem)
  {
    GuiElem e{GUI_BUTTON, Align::top_left, id, {elem}};
    e.props = GuiElem::ButtonProps{};
    return e;
  }

  [[nodiscard]] inline GuiElem guiButton(
    EventID id, Align align, const GuiElem& elem)
  {
    GuiElem e{GUI_BUTTON, align, id, {elem}};
    e.props = GuiElem::ButtonProps{};
    return e;
  }

  [[nodiscard]] inline GuiElem guiButton(EventID id, std::string_view text)
  {
    GuiElem e{GUI_BUTTON, Align::top_left, id, {guiLabel(Align::center, text)}};
    e.props = GuiElem::ButtonProps{};
    return e;
  }

  [[nodiscard]] inline GuiElem guiButton(
    EventID id, Align align, std::string_view text)
  {
    GuiElem e{GUI_BUTTON, align, id, {guiLabel(Align::center, text)}};
    e.props = GuiElem::ButtonProps{};
    return e;
  }

  // ButtonPress
  // (triggered on initial button press, holding repeats if repeat_delay >= 0)
  [[nodiscard]] inline GuiElem guiButtonPress(
    EventID id, int64_t repeat_delay, const GuiElem& elem)
  {
    GuiElem e{GUI_BUTTON_PRESS, Align::top_left, id, {elem}};
    e.props = GuiElem::ButtonProps{.repeatDelay = repeat_delay};
    return e;
  }

  [[nodiscard]] inline GuiElem guiButtonPress(
    EventID id, Align align, int64_t repeat_delay, const GuiElem& elem)
  {
    GuiElem e{GUI_BUTTON_PRESS, align, id, {elem}};
    e.props = GuiElem::ButtonProps{.repeatDelay = repeat_delay};
    return e;
  }

  [[nodiscard]] inline GuiElem guiButtonPress(
    EventID id, int64_t repeat_delay, std::string_view text)
  {
    GuiElem e{GUI_BUTTON_PRESS, Align::top_left, id,
              {guiLabel(Align::center, text)}};
    e.props = GuiElem::ButtonProps{.repeatDelay = repeat_delay};
    return e;
  }

  [[nodiscard]] inline GuiElem guiButtonPress(
    EventID id, Align align, int64_t repeat_delay, std::string_view text)
  {
    GuiElem e{GUI_BUTTON_PRESS, align, id, {guiLabel(Align::center, text)}};
    e.props = GuiElem::ButtonProps{.repeatDelay = repeat_delay};
    return e;
  }

  // Checkbox
  [[nodiscard]] inline GuiElem guiCheckbox(
    EventID id, bool set, const GuiElem& label)
  {
    GuiElem e{GUI_CHECKBOX, Align::left, id, {label}};
    e.props = GuiElem::CheckboxProps{.set = set};
    return e;
  }

  [[nodiscard]] inline GuiElem guiCheckbox(
    EventID id, Align align, bool set, const GuiElem& label)
  {
    GuiElem e{GUI_CHECKBOX, align, id, {label}};
    e.props = GuiElem::CheckboxProps{.set = set};
    return e;
  }

  [[nodiscard]] inline GuiElem guiCheckbox(
    EventID id, bool set, std::string_view label)
  {
    GuiElem e{GUI_CHECKBOX, Align::top_left, id, {guiLabel(Align::left, label)}};
    e.props = GuiElem::CheckboxProps{.set = set};
    return e;
  }

  [[nodiscard]] inline GuiElem guiCheckbox(
    EventID id, Align align, bool set, std::string_view label)
  {
    GuiElem e{GUI_CHECKBOX, align, id, {guiLabel(Align::left, label)}};
    e.props = GuiElem::CheckboxProps{.set = set};
    return e;
  }

  // Menu
  template<class... Elems>
  [[nodiscard]] inline GuiElem guiMenu(
    EventID id, std::string_view text, const Elems&... items)
  {
    return {GUI_MENU, Align::top_left, id,
            {guiLabel(Align::center, text),
             GuiElem{GUI_POPUP, Align::top_left, 0, {guiVFrame(items...)}}}};
  }

  [[nodiscard]] inline GuiElem guiMenuItem(int no, std::string_view text)
  {
    GuiElem e{GUI_MENU_ITEM, Align::justify, 0,
              {guiLabel(Align::center_left, text)}};
    e.props = GuiElem::ItemProps{.no = no};
    return e;
  }

  template<class... Elems>
  [[nodiscard]] inline GuiElem guiSubMenu(
    std::string_view text, const Elems&... items)
  {
    return {GUI_SUBMENU, Align::justify, 0,
            {guiLabel(Align::center_left, text),
             GuiElem{GUI_POPUP, Align::top_left, 0, {guiVFrame(items...)}}}};
  }

  // List Select
  template<class... Elems>
  [[nodiscard]] inline GuiElem guiListSelect(EventID id, const Elems&... items)
  {
    GuiElem e{GUI_LISTSELECT, Align::top_left, id,
              {guiLabel(""), // copy of label from selected item
               GuiElem{GUI_POPUP, Align::top_left, 0, {guiVFrame(items...)}}}};
    e.props = GuiElem::ItemProps{};
    return e;
  }

  template<class... Elems>
  [[nodiscard]] inline GuiElem guiListSelect(
    EventID id, Align align, const Elems&... items)
  {
    GuiElem e{GUI_LISTSELECT, align, id,
              {guiLabel(""), // copy of label from selected item
               GuiElem{GUI_POPUP, Align::top_left, 0, {guiVFrame(items...)}}}};
    e.props = GuiElem::ItemProps{};
    return e;
  }

  [[nodiscard]] inline GuiElem guiListSelectItem(int no, std::string_view text)
  {
    GuiElem e{GUI_LISTSELECT_ITEM, Align::justify, 0,
              {guiLabel(Align::center_left, text)}};
    e.props = GuiElem::ItemProps{.no = no};
    return e;
  }

  // Entry
  [[nodiscard]] inline GuiElem guiEntry(
    EventID id, Align align, EntryType type, float size,
    uint32_t maxChars, Align textAlign)
  {
    GuiElem e{GUI_ENTRY, align, id};
    GuiElem::EntryProps entry;
    if (type == ENTRY_CARDINAL || type == ENTRY_INTEGER
        || type == ENTRY_FLOAT) { entry.text = "0"; }
    entry.size = size;
    entry.maxChars = maxChars;
    entry.type = type;
    entry.align = textAlign;
    e.props = entry;
    return e;
  }

  [[nodiscard]] inline GuiElem guiTextEntry(
    EventID id, float size, uint32_t maxChars, Align textAlign = Align::left) {
    return guiEntry(id, Align::top_left, ENTRY_TEXT, size, maxChars, textAlign);
  }

  [[nodiscard]] inline GuiElem guiTextEntry(
    EventID id, Align align, float size, uint32_t maxChars,
    Align textAlign = Align::left) {
    return guiEntry(id, align, ENTRY_TEXT, size, maxChars, textAlign);
  }

  [[nodiscard]] inline GuiElem guiCardinalEntry(
    EventID id, float size, uint32_t maxChars, Align textAlign = Align::left) {
    return guiEntry(
      id, Align::top_left, ENTRY_CARDINAL, size, maxChars, textAlign);
  }

  [[nodiscard]] inline GuiElem guiCardinalEntry(
    EventID id, Align align, float size, uint32_t maxChars,
    Align textAlign = Align::left) {
    return guiEntry(id, align, ENTRY_CARDINAL, size, maxChars, textAlign);
  }

  [[nodiscard]] inline GuiElem guiIntegerEntry(
    EventID id, float size, uint32_t maxChars, Align textAlign = Align::left) {
    return guiEntry(
      id, Align::top_left, ENTRY_INTEGER, size, maxChars, textAlign);
  }

  [[nodiscard]] inline GuiElem guiIntegerEntry(
    EventID id, Align align, float size, uint32_t maxChars,
    Align textAlign = Align::left) {
    return guiEntry(id, align, ENTRY_INTEGER, size, maxChars, textAlign);
  }

  [[nodiscard]] inline GuiElem guiFloatEntry(
    EventID id, float size, uint32_t maxChars, Align textAlign = Align::left) {
    return guiEntry(id, Align::top_left, ENTRY_FLOAT, size, maxChars, textAlign);
  }

  [[nodiscard]] inline GuiElem guiFloatEntry(
    EventID id, Align align, float size, uint32_t maxChars,
    Align textAlign = Align::left) {
    return guiEntry(id, align, ENTRY_FLOAT, size, maxChars, textAlign);
  }

  [[nodiscard]] inline GuiElem guiPasswordEntry(
    EventID id, float size, uint32_t maxChars, Align textAlign = Align::left) {
    return guiEntry(
      id, Align::top_left, ENTRY_PASSWORD, size, maxChars, textAlign);
  }

  [[nodiscard]] inline GuiElem guiPasswordEntry(
    EventID id, Align align, float size, uint32_t maxChars,
    Align textAlign = Align::left) {
    return guiEntry(id, align, ENTRY_PASSWORD, size, maxChars, textAlign);
  }

  // Image
  [[nodiscard]] inline GuiElem guiImage(
    float w, float h, TextureID tid, Vec2 t0, Vec2 t1)
  {
    GuiElem e{GUI_IMAGE, Align::top_left, 0};
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
    return {GUI_TITLEBAR, Align::hjustify, 0};
  }

  [[nodiscard]] inline GuiElem guiTitleBar(std::string_view text) {
    return {GUI_TITLEBAR, Align::hjustify, 0, {guiLabel(Align::center, text)}};
  }

  // VTitleBar
  [[nodiscard]] inline GuiElem guiVTitleBar() {
    return {GUI_TITLEBAR, Align::vjustify, 0};
  }

  [[nodiscard]] inline GuiElem guiVTitleBar(std::string_view text) {
    return {GUI_TITLEBAR, Align::vjustify, 0, {guiVLabel(Align::center, text)}};
  }
}
