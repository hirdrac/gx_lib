//
// NormalTest.cc
// Copyright (C) 2025 Richard Bradley
//

#include "gx/Normal.hh"
#include "gx/Print.hh"
#include <cassert>

#ifdef NDEBUG
#error "can't run test with NDEBUG"
#endif

using namespace gx;


void test_pack_unpack(const Vec3& n)
{
  const Vec3 n2 = unpackNormal(packNormal(n));
  //println(n, " == ", n2);
  assert(n == n2);
}

int main(int argc, char** argv)
{
  // check values -1,0,1 are perfectly encoded/decoded
  test_pack_unpack({-1,  0,  1});
  test_pack_unpack({ 0,  1, -1});
  test_pack_unpack({ 1, -1,  0});
  return 0;
}
