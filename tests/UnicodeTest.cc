//
// UnicodeTest.cc
// Copyright (C) 2024 Richard Bradley
//

#include "gx/Unicode.hh"
#include <cassert>
using namespace gx;


int main(int argc, char** argv)
{
  const unsigned char invalid_bytes[] = {
    0xc0, 0xc1, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
  };

  for (int ch : invalid_bytes) {
    const char str[] = {char(ch)};
    UTF8Iterator itr{std::string_view{str, 1}};

    assert(itr.get() == -1);
    assert(!itr.done());
    assert(!itr.next());
    assert(itr.get() == 0);
    assert(itr.done());
  }

  return 0;
}
