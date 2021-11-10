//
// StringUtilTest.cc
// Copyright (C) 2021 Richard Bradley
//

#include "gx/StringUtil.hh"
#include <cassert>

#ifdef NDEBUG
#error "can't run test with NDEBUG"
#endif


void test_formatInt()
{
  assert(gx::formatInt(1) == "1");
  assert(gx::formatInt(12) == "12");
  assert(gx::formatInt(123) == "123");
  assert(gx::formatInt(1234) == "1,234");
  assert(gx::formatInt(12345) == "12,345");
  assert(gx::formatInt(123456) == "123,456");
  assert(gx::formatInt(1234567) == "1,234,567");
  assert(gx::formatInt(12345678) == "12,345,678");
  assert(gx::formatInt(123456789) == "123,456,789");
  assert(gx::formatInt(1234567890) == "1,234,567,890");
  assert(gx::formatInt(12345678901) == "12,345,678,901");

  assert(gx::formatInt(-1) == "-1");
  assert(gx::formatInt(-12) == "-12");
  assert(gx::formatInt(-123) == "-123");
  assert(gx::formatInt(-1234) == "-1,234");
  assert(gx::formatInt(-12345) == "-12,345");
  assert(gx::formatInt(-123456) == "-123,456");
  assert(gx::formatInt(-1234567) == "-1,234,567");
  assert(gx::formatInt(-12345678) == "-12,345,678");
  assert(gx::formatInt(-123456789) == "-123,456,789");
  assert(gx::formatInt(-1234567890) == "-1,234,567,890");
  assert(gx::formatInt(-12345678901) == "-12,345,678,901");

  assert(gx::formatInt(123456789,'.') == "123.456.789");
  assert(gx::formatInt(-123456789,'_') == "-123_456_789");
}

int main(int argc, char** argv)
{
  test_formatInt();
  return 0;
}
