//
// ColorTest.cc
// Copyright (C) 2021 Richard Bradley
//

#include "gx/Color.hh"
#include <cassert>

#ifdef NDEBUG
#error "can't run test with NDEBUG"
#endif

using gx::Color;
using gx::RGBA8;
using gx::packRGBA8;
using gx::unpackRGBA8;


void check(RGBA8 c, uint32_t r, uint32_t g, uint32_t b, uint32_t a)
{
  assert((c & 255) == r);
  assert(((c >> 8) & 255) == g);
  assert(((c >> 16) & 255) == b);
  assert(((c >> 24) & 255) == a);
}

void test_unpack_pack()
{
  // check rounding of int -> float ->int for each channel

  // red
  for (uint32_t r = 0; r < 256; ++r) {
    RGBA8 c0 = r;
    Color c1 = unpackRGBA8(c0);
    RGBA8 c2 = packRGBA8(c1);

    check(c2,r,0,0,0);
  }

  // green
  for (uint32_t g = 0; g < 256; ++g) {
    RGBA8 c0 = g << 8;
    Color c1 = unpackRGBA8(c0);
    RGBA8 c2 = packRGBA8(c1);

    check(c2,0,g,0,0);
  }

  // blue
  for (uint32_t b = 0; b < 256; ++b) {
    RGBA8 c0 = b << 16;
    Color c1 = unpackRGBA8(c0);
    RGBA8 c2 = packRGBA8(c1);

    check(c2,0,0,b,0);
  }

  // alpha
  for (uint32_t a = 0; a < 256; ++a) {
    RGBA8 c0 = a << 24;
    Color c1 = unpackRGBA8(c0);
    RGBA8 c2 = packRGBA8(c1);

    check(c2,0,0,0,a);
  }    
}

int main(int argc, char** argv)
{
  test_unpack_pack();
  return 0;
}
