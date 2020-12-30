//
// gx/Align.hh
// Copyright (C) 2020 Richard Bradley
//

#pragma once


namespace gx {
  // **** Types ****
    enum AlignEnum {
    ALIGN_UNSPECIFIED = 0,

    // vertical alignments
    ALIGN_TOP = 1<<0,
    ALIGN_BOTTOM = 1<<1,
    ALIGN_VCENTER = ALIGN_TOP | ALIGN_BOTTOM,
    ALIGN_VJUSTIFY = 1<<2,

    // horizontal alignments
    ALIGN_LEFT = 1<<3,
    ALIGN_RIGHT = 1<<4,
    ALIGN_HCENTER = ALIGN_LEFT | ALIGN_RIGHT,
    ALIGN_HJUSTIFY = 1<<5,

    // combined vertical & horizontal alignments
    ALIGN_TOP_LEFT = ALIGN_TOP | ALIGN_LEFT,
    ALIGN_TOP_RIGHT = ALIGN_TOP | ALIGN_RIGHT,
    ALIGN_TOP_CENTER = ALIGN_TOP | ALIGN_HCENTER,
    ALIGN_BOTTOM_LEFT = ALIGN_BOTTOM | ALIGN_LEFT,
    ALIGN_BOTTOM_RIGHT = ALIGN_BOTTOM | ALIGN_RIGHT,
    ALIGN_BOTTOM_CENTER = ALIGN_BOTTOM | ALIGN_HCENTER,
    ALIGN_CENTER_LEFT = ALIGN_VCENTER | ALIGN_LEFT,
    ALIGN_CENTER_RIGHT = ALIGN_VCENTER | ALIGN_RIGHT,
    ALIGN_CENTER_CENTER = ALIGN_VCENTER | ALIGN_HCENTER,
    ALIGN_CENTER = ALIGN_VCENTER | ALIGN_HCENTER,
    ALIGN_JUSTIFY = ALIGN_VJUSTIFY | ALIGN_HJUSTIFY
  };

  // **** Functions ****
  constexpr AlignEnum VAlign(AlignEnum a) {
    return AlignEnum(a & (ALIGN_TOP | ALIGN_BOTTOM | ALIGN_VJUSTIFY)); }

  constexpr AlignEnum HAlign(AlignEnum a) {
    return AlignEnum(a & (ALIGN_LEFT | ALIGN_RIGHT | ALIGN_HJUSTIFY)); }
}
