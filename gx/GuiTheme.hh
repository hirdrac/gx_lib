//
// gx/GuiTheme.hh
// Copyright (C) 2025 Richard Bradley
//

#pragma once
#include "Style.hh"
#include "Color.hh"
#include "Types.hh"


namespace gx {
  struct GuiTheme;
}

struct gx::GuiTheme
{
  const Font* font = nullptr;
  Style panel = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.2f,.2f,.2f,1.0f), 0,
    packRGBA8(.18f,.18f,.18f,1), Style::border_1px};
  Style titlebar = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.3f,.3f,.3f,1.0f)};

  Style button = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.4f,.4f,.4f,1.0f)};
  Style buttonHover = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.8f,.4f,.4f,1.0f)};
  Style buttonPress = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.8f,.8f,.8f,1.0f)};
  Style buttonHold = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.6f,.6f,.6f,1.0f)};
  Style buttonDisable = {
    packRGBA8(.6f,.6f,.6f,1.0f), packRGBA8(.4f,.4f,.4f,1.0f)};

  Style checkbox = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.4f,.4f,.4f,1.0f),
    0,0, Style::no_edge, Style::solid, 8.0f};
  Style checkboxHover = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.8f,.4f,.4f,1.0f),
    0,0, Style::no_edge, Style::solid, 8.0f};
  Style checkboxPress = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.8f,.8f,.8f,1.0f),
    0,0, Style::no_edge, Style::solid, 8.0f};
  Style checkboxHold = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.6f,.6f,.6f,1.0f),
    0,0, Style::no_edge, Style::solid, 8.0f};
  Style checkboxDisable = {
    packRGBA8(.5f,.5f,.5f,1.0f), packRGBA8(.4f,.4f,.4f,1.0f),
    0,0, Style::no_edge, Style::solid, 8.0f};

  Style menuButton = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), 0};
  Style menuButtonHover = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.8f,.4f,.4f,1.0f)};
  Style menuButtonOpen = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.6f,.6f,.6f,1.0f), 0,
    packRGBA8(1.0f,1.0f,1.0f,1.0f), Style::underline_2px};
  Style menuFrame = {
    packRGBA8(.5f,.5f,.5f,1.0f), packRGBA8(0.0f,0.0f,0.0f,.85f)};
  Style menuItem = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), 0};
  Style menuItemSelect = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.8f,.8f,.8f,1.0f)};
  Style menuItemDisable = {
    packRGBA8(.5f,.5f,.5f,1.0f), 0};

  Style listSelect = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.4f,.4f,.4f,1.0f)};
  Style listSelectHover = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.8f,.4f,.4f,1.0f)};
  Style listSelectOpen = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.6f,.6f,.6f,1.0f), 0,
    packRGBA8(1.0f,1.0f,1.0f,1.0f), Style::underline_2px};
  Style listSelectDisable = {
    packRGBA8(.6f,.6f,.6f,1.0f), packRGBA8(.4f,.4f,.4f,1.0f)};
  Style listSelectFrame = {
    packRGBA8(.5f,.5f,.5f,1.0f), packRGBA8(0.0f,0.0f,0.0f,.85f)};
  Style listSelectItem = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), 0};
  Style listSelectItemSelect = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.8f,.8f,.8f,1.0f)};
  Style listSelectItemDisable = {
    packRGBA8(.5f,.5f,.5f,1.0f), 0};

  Style entry = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(0.0f,0.0f,.2f,1.0f)};
  Style entryFocus = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f),
    packRGBA8(0.0f,0.0f,.2f,1.0f), packRGBA8(.15f,.15f,.35f,1.0f), 0,
    Style::no_edge, Style::vgradient};
  Style entryDisable = {
    packRGBA8(.5f,.5f,.5f,1.0f), packRGBA8(.15f,.15f,.2f,1.0f)};

  uint16_t entryLeftMargin = 10;
  uint16_t entryRightMargin = 10;
  uint16_t entryTopMargin = 4;
  uint16_t entryBottomMargin = 3;

  int32_t checkCode = 'X';
  int16_t checkXOffset = 0;
  int16_t checkYOffset = 2;

  int32_t passwordCode = 8226; // U+2022 (bullet)
  int32_t subMenuCode = '>';
  //int32_t subMenuCode = 9658; // LiberationSans-Regular
  int32_t listSelectCode = 8711; // U+2207
  //int32_t listSelectCode = 9660; // LiberationSans-Regular
  int32_t listSelectOpenCode = 8710; // U+2206
  int32_t listSelectItemCode = 0; // disabled by default

  RGBA8 cursorColor = packRGBA8(1.0f,1.0f,.6f,1.0f);
  RGBA8 textSelectColor = packRGBA8(.5f,.5f,.5f,1.0f);
  uint32_t multiClickTime = 300000; // .3 sec
  uint32_t cursorBlinkTime = 400000; // .4 sec
  uint16_t cursorWidth = 3;

  uint16_t panelBorder = 8;
  uint16_t popupBorder = 4; // for menuFrame,listSelectFrame
  uint16_t lineBorder = 4;
  uint16_t border = 6;
  uint16_t frameSpacing = 4;
  uint16_t textSpacing = 1;
  uint16_t lineWidth = 2;
  uint16_t emptyHeight = 12;
  uint16_t emptyWidth = 12;
};
