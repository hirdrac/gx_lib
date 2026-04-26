//
// StringUtilTest.cc
// Copyright (C) 2026 Richard Bradley
//

#include "gx/StringUtil.hh"
#include <limits>
#include <cassert>
using namespace gx;

#ifdef NDEBUG
#error "can't run test with NDEBUG"
#endif


void test_formatInt()
{
  assert(formatInt(1) == "1");
  assert(formatInt(12) == "12");
  assert(formatInt(123) == "123");
  assert(formatInt(1234) == "1,234");
  assert(formatInt(12345) == "12,345");
  assert(formatInt(123456) == "123,456");
  assert(formatInt(1234567) == "1,234,567");
  assert(formatInt(12345678) == "12,345,678");
  assert(formatInt(123456789) == "123,456,789");
  assert(formatInt(1234567890) == "1,234,567,890");
  assert(formatInt(12345678901) == "12,345,678,901");

  assert(formatInt(-1) == "-1");
  assert(formatInt(-12) == "-12");
  assert(formatInt(-123) == "-123");
  assert(formatInt(-1234) == "-1,234");
  assert(formatInt(-12345) == "-12,345");
  assert(formatInt(-123456) == "-123,456");
  assert(formatInt(-1234567) == "-1,234,567");
  assert(formatInt(-12345678) == "-12,345,678");
  assert(formatInt(-123456789) == "-123,456,789");
  assert(formatInt(-1234567890) == "-1,234,567,890");
  assert(formatInt(-12345678901) == "-12,345,678,901");

  assert(formatInt(123456789,'.') == "123.456.789");
  assert(formatInt(-123456789,'_') == "-123_456_789");

  // special cases
  assert(formatInt(0) == "0");
  assert(formatInt(std::numeric_limits<int8_t>::min()) == "-128");
  assert(formatInt(std::numeric_limits<int16_t>::min()) == "-32,768");
  assert(formatInt(std::numeric_limits<int32_t>::min()) == "-2,147,483,648");
  assert(formatInt(std::numeric_limits<int64_t>::min()) == "-9,223,372,036,854,775,808");
}

void test_trimSpaces()
{
  static_assert(trimSpaces("TEST 1") == "TEST 1");
  static_assert(trimSpaces("  TEST 2") == "TEST 2");
  static_assert(trimSpaces("TEST 3  ") == "TEST 3");
  static_assert(trimSpaces("  TEST 4  ") == "TEST 4");
}

void test_LineIterator()
{
  LineIterator itr{"line 1\n\nline 3"};
  assert(itr && *itr == "line 1"); ++itr;
  assert(itr && *itr == ""); ++itr;
  assert(itr && *itr == "line 3"); ++itr;
  assert(!itr);
}

int main(int argc, char** argv)
{
  test_formatInt();
  test_trimSpaces();
  test_LineIterator();
  return 0;
}
