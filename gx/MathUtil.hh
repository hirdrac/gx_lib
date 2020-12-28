//
// gx/MathUtil.hh
// Copyright (C) 2020 Richard Bradley
//
// fun and useful numeric constants and
// various numeric functions needing a home
//

#pragma once
#include <type_traits>
#include <cmath>


namespace gx {
  // **** Constants ****
  template<class T>
#ifndef M_PI
  constexpr T PI = static_cast<T>(3.14159265358979323846);
#else
  constexpr T PI = static_cast<T>(M_PI);
#endif

  template<class T>
#ifndef M_PI_2
  constexpr T PI_2 = static_cast<T>(1.57079632679489661923);
#else
  constexpr T PI_2 = static_cast<T>(M_PI_2);  // pi/2
#endif

  template<class T>
#ifndef M_PI_4
  constexpr T PI_4 = static_cast<T>(0.78539816339744830962);
#else
  constexpr T PI_4 = static_cast<T>(M_PI_4);  // pi/4
#endif

  template<class T>
  constexpr T DEG_TO_RAD = PI<T> / static_cast<T>(180);

  template<class T>
  constexpr T RAD_TO_DEG = static_cast<T>(180) / PI<T>;

  template<class T>
  constexpr T VERY_SMALL = static_cast<T>(1.0e-7);


  // **** Functions ****
  template<typename fltType>
  constexpr fltType DegToRad(const fltType& deg)
  {
    return deg * DEG_TO_RAD<fltType>;
  }

  template<typename fltType>
  constexpr fltType RadToDeg(const fltType& rad)
  {
    return rad * RAD_TO_DEG<fltType>;
  }

  template<typename fltType>
  constexpr bool IsZero(const fltType& x)
  {
    return (x > -VERY_SMALL<fltType>) && (x < VERY_SMALL<fltType>);
  }

  template<typename fltType>
  constexpr bool IsOne(const fltType& x)
  {
    return (x > (static_cast<fltType>(1) - VERY_SMALL<fltType>))
      && (x < (static_cast<fltType>(1) + VERY_SMALL<fltType>));
  }

  template<typename fltType>
  constexpr bool IsPositive(const fltType& x)
  {
    return (x >= VERY_SMALL<fltType>);
  }

  template<typename fltType>
  constexpr bool IsNegative(const fltType& x)
  {
    return (x <= -VERY_SMALL<fltType>);
  }

  template<typename fltType>
  constexpr bool IsEqual(const fltType& x, const fltType& y)
  {
    return IsZero(x-y);
  }

  template<typename fltType>
  constexpr bool IsLess(const fltType& x, const fltType& y)
  {
    return IsNegative(x-y);
  }

  template<typename fltType>
  constexpr bool IsGreater(const fltType& x, const fltType& y)
  {
    return IsPositive(x-y);
  }

  template<typename intType>
  constexpr bool IsPowerOf2(const intType& x)
  {
    static_assert(std::is_integral_v<intType>);
    return (x & (x - 1)) == 0;
  }

  template<typename fltType>
  constexpr fltType Lerp(const fltType& a, const fltType& b, const fltType& s)
  {
    // use std::lerp() for C++20
    if (s <= 0) {
      return a;
    } else if (s >= static_cast<fltType>(1)) {
      return b;
    } else {
      return a + ((b - a) * s);
    }
  }

  template<typename numType>
  constexpr numType Sqr(const numType& x) { return x * x; }

  template<typename numType>
  constexpr numType iPow(numType x, int y)
  {
    if (y < 0) { return 0; }

    numType val = 1;
    while (y) {
      if (y & 1) { val *= x; }
      y >>= 1;
      x *= x;
    }

    return val;
  }

  template<typename numType>
  constexpr int Sgn(const numType& x)
  {
    return (x < 0) ? -1 : ((x > 0) ? 1 : 0);
  }

  template<typename numType>
  constexpr numType Abs(const numType& x)
  {
    // constexpr version of std::abs
    // NOTE: Abs(-MAX_INT) is undefined
    return (x < 0) ? -x : x;
  }
}
