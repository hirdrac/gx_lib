//
// gx/Style.hh
// Copyright (C) 2026 Richard Bradley
//

// TODO: add top/bottom/left/right texture coords
// TODO: add texture FillType

#pragma once
#include "Color.hh"
#include "Types.hh"


struct gx::Style
{
  enum EdgeType {
    no_edge       = 0,
    border_1px    = 1,
    border_2px    = 2,
    underline_1px = 3,
    underline_2px = 4,
    overline_1px  = 5,
    overline_2px  = 6,
  };

  enum FillType {
    no_fill   = 0,
    solid     = 1, // fillColor only (fillColor2 ignored)
    hgradient = 2, // fillColor(left), fillColor(right)
    vgradient = 3, // fillColor(top), fillColor2(bottom)
  };

  enum ShapeType {
    square = 0,
    roundedTopLeftCorner = 1,
    roundedTopRightCorner = 2,
    roundedBottomLeftCorner = 4,
    roundedBottomRightCorner = 8,
    roundedTopCorners = roundedTopLeftCorner | roundedTopRightCorner,
    roundedBottomCorners = roundedBottomLeftCorner | roundedBottomRightCorner,
    roundedLeftCorners = roundedTopLeftCorner | roundedBottomLeftCorner,
    roundedRightCorners = roundedTopRightCorner | roundedBottomRightCorner,
    roundedCorners = roundedTopCorners | roundedBottomCorners,
  };

  RGBA8 textColor;
  RGBA8 fillColor;
  RGBA8 fillColor2 = 0;
  RGBA8 edgeColor = 0;
  EdgeType edge = border_1px;
  FillType fill = solid;
  ShapeType shape = roundedCorners;
  float cornerRadius = 0.0f;  // default to unrounded
  uint16_t cornerSegments = 3;
};
