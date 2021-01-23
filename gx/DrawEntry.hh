//
// gx/DrawEntry.hh
// Copyright (C) 2021 Richard Bradley
//

#pragma once
#include "Types.hh"


namespace gx {
  enum DrawCmd : uint32_t {
    // control/state commands
    CMD_color,     // <cmd c> (2)
    CMD_lineWidth, // <cmd w> (2)
    CMD_texture,   // <cmd tid> (2)

    // drawing commands
    CMD_line,        // <cmd (x y)x2> (5)
    CMD_triangle,    // <cmd (x y)x3> (7)
    CMD_triangleT,   // <cmd (x y tx ty)x3> (13)
    CMD_triangleC,   // <cmd (x y c)x3> (10)
    CMD_triangleTC,  // <cmd (x y tx ty c)x3> (16)
    CMD_quad,        // <cmd (x y)x4> (9)
    CMD_quadT,       // <cmd (x y tx ty)x4> (17)
    CMD_quadC,       // <cmd (x y c)x4> (13)
    CMD_quadTC,      // <cmd (x y tx ty c)x4> (21)
    CMD_rectangle,   // <cmd (x y)x2> (5)
    CMD_rectangleT,  // <cmd (x y tx ty)x2> (9)
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
