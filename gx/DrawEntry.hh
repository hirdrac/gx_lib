//
// gx/DrawEntry.hh
// Copyright (C) 2021 Richard Bradley
//

#pragma once
#include "Types.hh"


namespace gx {
  enum DrawCmd : uint32_t {
    // state / control commands
    CMD_capabilities, // <cmd c> (2)
    CMD_transform,    // <cmd view proj> (33)
    CMD_color,        // <cmd c> (2)
    CMD_modColor,     // <cmd c> (2)
    CMD_texture,      // <cmd id> (2)
    CMD_lineWidth,    // <cmd w> (2)

    // drawing commands
    CMD_line2,        // <cmd (x y)x2> (5)
    CMD_line3,        // <cmd (x y z)x2> (7)
    CMD_triangle2,    // <cmd (x y)x3> (7)
    CMD_triangle3,    // <cmd (x y z)x3> (10)
    CMD_triangle2T,   // <cmd (x y s t)x3> (13)
    CMD_triangle3T,   // <cmd (x y z s t)x3> (16)
    CMD_triangle2C,   // <cmd (x y c)x3> (10)
    CMD_triangle3C,   // <cmd (x y z c)x3> (13)
    CMD_triangle2TC,  // <cmd (x y s t c)x3> (16)
    CMD_triangle3TC,  // <cmd (x y z s t c)x3> (19)
    CMD_quad2,        // <cmd (x y)x4> (9)
    CMD_quad3,        // <cmd (x y z)x4> (13)
    CMD_quad2T,       // <cmd (x y s t)x4> (17)
    CMD_quad3T,       // <cmd (x y z s t)x4> (21)
    CMD_quad2C,       // <cmd (x y c)x4> (13)
    CMD_quad3C,       // <cmd (x y z c)x4> (17)
    CMD_quad2TC,      // <cmd (x y s t c)x4> (21)
    CMD_quad3TC,      // <cmd (x y z s t c)x4> (25)
    CMD_rectangle,    // <cmd (x y)x2> (5)
    CMD_rectangleT,   // <cmd (x y s t)x2> (9)
  };

  struct DrawEntry {
    union {
      DrawCmd  cmd;
      float    fval;
      int32_t  ival;
      uint32_t uval;
    };

    DrawEntry(DrawCmd c) : cmd(c) { }
    DrawEntry(float f) : fval(f) { }
    DrawEntry(int32_t i) : ival(i) { }
    DrawEntry(uint32_t u) : uval(u) { }
  };
}
