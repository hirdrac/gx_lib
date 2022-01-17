//
// gx/GuiTheme.hh
// Copyright (C) 2022 Richard Bradley
//

#pragma once
#include "Color.hh"
#include "Types.hh"


namespace gx {
  class Font;
  struct GuiTheme;
}


struct gx::GuiTheme
{
  enum EdgeTypeEnum : int32_t {
    EDGE_NONE          = 0,
    EDGE_BORDER_1px    = 1,
    EDGE_BORDER_2px    = 2,
    EDGE_UNDERLINE_1px = 3,
    EDGE_UNDERLINE_2px = 4
  };

  enum BackgroundTypeEnum : int32_t {
    BG_NONE      = 0,
    BG_SOLID     = 1, // backgroundColor only
    BG_VGRADIENT = 2, // backgroundColor(top), backgroundColor2(bottom)
  };

  struct Style {
    RGBA8 textColor;
    RGBA8 backgroundColor;
    RGBA8 backgroundColor2 = 0;
    RGBA8 edgeColor = 0;
    EdgeTypeEnum edgeType = EDGE_BORDER_1px;
    BackgroundTypeEnum backgroundType = BG_SOLID;
  };

  const Font* font = nullptr;
  Style panel = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.2f,.2f,.2f,1.0f)};
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
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.4f,.4f,.4f,1.0f)};
  Style checkboxHover = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.8f,.4f,.4f,1.0f)};
  Style checkboxPress = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.8f,.8f,.8f,1.0f)};
  Style checkboxHold = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.6f,.6f,.6f,1.0f)};
  Style checkboxDisable = {
    packRGBA8(.5f,.5f,.5f,1.0f), packRGBA8(.4f,.4f,.4f,1.0f)};

  Style menuButton = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), 0};
  Style menuButtonHover = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.8f,.4f,.4f,1.0f)};
  Style menuButtonOpen = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.6f,.6f,.6f,1.0f), 0,
    packRGBA8(1.0f,1.0f,1.0f,1.0f), EDGE_UNDERLINE_2px};
  Style menuFrame = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(0.0f,0.0f,0.0f,.85f)};
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
    packRGBA8(1.0f,1.0f,1.0f,1.0f), EDGE_UNDERLINE_2px};
  Style listSelectDisable = {
    packRGBA8(.6f,.6f,.6f,1.0f), packRGBA8(.4f,.4f,.4f,1.0f)};
  Style listSelectFrame = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(0.0f,0.0f,0.0f,.85f)};
  Style listSelectItemSelect = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(.8f,.8f,.8f,1.0f)};
  Style listSelectItemDisable = {
    packRGBA8(.5f,.5f,.5f,1.0f), 0};

  Style entry = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f), packRGBA8(0.0f,0.0f,.2f,1.0f)};
  Style entryFocus = {
    packRGBA8(1.0f,1.0f,1.0f,1.0f),
    packRGBA8(0.0f,0.0f,.2f,1.0f), packRGBA8(.15f,.15f,.35f,1.0f), 0,
    EDGE_NONE, BG_VGRADIENT};
  Style entryDisable = {
    packRGBA8(.5f,.5f,.5f,1.0f), packRGBA8(.15f,.15f,.2f,1.0f)};

  uint16_t entryLeftMargin = 6;
  uint16_t entryRightMargin = 6;
  uint16_t entryTopMargin = 2;
  uint16_t entryBottomMargin = 2;

  int32_t checkCode = 'X';
  int16_t checkXOffset = 0;
  int16_t checkYOffset = 2;

  int32_t passwordCode = 8226; // U+2022 (bullet)
  int32_t subMenuCode = '>';
  int32_t listSelectCode = 8711; // U+2207

  RGBA8 cursorColor = packRGBA8(1.0f,1.0f,.6f,1.0f);
  uint32_t cursorBlinkTime = 400000; // 1 sec
  uint16_t cursorWidth = 3;

  uint16_t panelBorder = 8;
  uint16_t menuFrameBorder = 4;
  uint16_t lineBorder = 4;
  uint16_t border = 6;
  uint16_t frameSpacing = 4;
  uint16_t textSpacing = 1;
  uint16_t lineWidth = 2;
  uint16_t titlebarMinHeight = 12;
  uint16_t titlebarMinWidth = 12;
};
