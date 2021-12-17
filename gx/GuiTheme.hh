//
// gx/GuiTheme.hh
// Copyright (C) 2021 Richard Bradley
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

  uint16_t entryLeftMargin = 6;
  uint16_t entryRightMargin = 6;
  uint16_t entryTopMargin = 2;
  uint16_t entryBottomMargin = 2;

  int32_t checkCode = 'X';
  int16_t checkXOffset = 0;
  int16_t checkYOffset = 2;

  int32_t passwordCode = 8226; // U+2022 (bullet)
  int32_t subMenuCode = '>';

  RGBA8 cursorColor = packRGBA8(1.0f,1.0f,.6f,1.0f);
  uint32_t cursorBlinkTime = 400000; // 1 sec
  uint16_t cursorWidth = 3;

  uint16_t border = 6;
  uint16_t frameSpacing = 4;
  uint16_t textSpacing = 1;
  uint16_t lineWidth = 2;
};
