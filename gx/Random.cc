//
// gx/Random.cc
// Copyright (C) 2026 Richard Bradley
//

#include "Random.hh"


namespace {
  std::random_device rd;
}

uint32_t gx::genRandomSeed32()
{
  return rd();
}

uint64_t gx::genRandomSeed64()
{
  return uint64_t{rd()} | (uint64_t{rd()} << 32);
}
