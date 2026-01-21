//
// gx/Align.hh
// Copyright (C) 2026 Richard Bradley
//

#pragma once


namespace gx {
  // **** Types ****
  enum class Align : int {
    unspecified = 0,

    // vertical alignments
    top = 1<<0,
    bottom = 1<<1,
    vcenter = top | bottom,
    vjustify = 1<<2,

    // horizontal alignments
    left = 1<<3,
    right = 1<<4,
    hcenter = left | right,
    hjustify = 1<<5,

    // combined vertical & horizontal alignments
    top_left = top | left,
    top_right = top | right,
    top_center = top | hcenter,
    bottom_left = bottom | left,
    bottom_right = bottom | right,
    bottom_center = bottom | hcenter,
    center_left = vcenter | left,
    center_right = vcenter | right,
    center_center = vcenter | hcenter,
    center = vcenter | hcenter,
    justify = vjustify | hjustify,
  };

  // **** Functions ****
  [[nodiscard]] constexpr Align vAlign(Align a) {
    return Align(int(a) & (int(Align::top) | int(Align::bottom) | int(Align::vjustify))); }

  [[nodiscard]] constexpr Align hAlign(Align a) {
    return Align(int(a) & (int(Align::left) | int(Align::right) | int(Align::hjustify))); }

  [[nodiscard]] constexpr bool hjustified(Align a) {
    return int(a) & int(Align::hjustify); }

  [[nodiscard]] constexpr bool vjustified(Align a) {
    return int(a) & int(Align::vjustify); }

  [[nodiscard]] constexpr bool justified(Align a) {
    return int(a) & int(Align::justify); }
}
