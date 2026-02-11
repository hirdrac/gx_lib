//
// MathUtilTest.cc
// Copyright (C) 2026 Richard Bradley
//

#include "gx/MathUtil.hh"
#include <cassert>


void test_sign()
{
  // static tests
  static_assert(gx::sign(0) == 0);
  static_assert(gx::sign(-99) == -1);
  static_assert(gx::sign(99) == 1);
}

void test_abs()
{
  // static tests
  static_assert(gx::abs(static_cast<signed char>(-128)) == 128);
  static_assert(gx::abs(static_cast<short>(-32768)) == 32768);
}


int main(int argc, char** argv)
{
  test_sign();
  test_abs();
  return 0;
}
