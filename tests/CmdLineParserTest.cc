//
// CmdLineParserTest.cc
// Copyright (C) 2024 Richard Bradley
//

#include "gx/CmdLineParser.hh"
#include "gx/Print.hh"
#include <cassert>
using namespace gx;

int main(int argc, char** argv)
{
  {
    const char* args[] = {"", "-a", "23", "99"};
    CmdLineParser p{4, args};
    assert(p.option());
    int val = -1;
    assert(!p.option('b',""));
    assert(!p.option('b',"", val));
    assert(p.option('a',""));
    assert(p.option('a',"",val) && val == 23); ++p;
    assert(p.arg() == "99");
  }

  return 0;
}
