//
// CmdLineParserTest.cc
// Copyright (C) 2025 Richard Bradley
//

#include "gx/CmdLineParser.hh"
#include <cassert>
using namespace gx;

int main(int argc, char** argv)
{
  {
    const char* args[] = {"", "-a", "23", "99"};
    CmdLineParser p{4, args};
    assert(!p.done());
    assert(p.option());
    int val = -1;
    assert(!p.option('b',""));
    assert(!p.option('b',"", val));
    assert(p.option('a',""));
    assert(p.option('a',"",val) && val == 23);
    assert(p.next());
    assert(!p.done());
    assert(p.arg() == "99");
    assert(!p.next());
    assert(p.done());
  }

  return 0;
}
