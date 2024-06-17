//
// gx/Assert.hh
// Copyright (C) 2024 Richard Bradley
//

// Macros (both disabled with NDEBUG or GX_NDEBUG)
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// GX_ASSERT()
//   same as assert() but can be disabled without disabling regular assert()
// GX_ASSERT_DEBUG()
//   same as assert() but disabled by default, enable with GX_DEBUG_ASSERT
//   (use for expensive asserts only needed for extreme debugging)


#pragma once
#include <cassert>

#ifdef GX_NDEBUG
#  define GX_ASSERT(...) static_cast<void>(0)
#  define GX_ASSERT_DEBUG(...) static_cast<void>(0)
#else
#  define GX_ASSERT(...) assert(__VA_ARGS__)
#  ifdef GX_DEBUG_ASSERT
#    define GX_ASSERT_DEBUG(...) assert(__VA_ARGS__)
#  else
#    define GX_ASSERT_DEBUG(...) static_cast<void>(0)
#  endif
#endif
