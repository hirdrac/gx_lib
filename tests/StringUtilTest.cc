//
// StringUtilTest.cc
// Copyright (C) 2024 Richard Bradley
//

#include "gx/StringUtil.hh"
#include <limits>
#include <cassert>

#ifdef NDEBUG
#error "can't run test with NDEBUG"
#endif


void test_formatInt()
{
  using gx::formatInt;

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

int main(int argc, char** argv)
{
  test_formatInt();
  return 0;
}
