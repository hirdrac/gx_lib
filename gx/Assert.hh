//
// gx/Assert.hh
// Copyright (C) 2022 Richard Bradley
//

#pragma once
#include <cassert>

#ifdef GX_NDEBUG
#define GX_ASSERT(expr) static_cast<void>(0)
#else
#define GX_ASSERT(expr) assert(expr)
#endif
