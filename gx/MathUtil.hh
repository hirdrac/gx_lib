//
// gx/MathUtil.hh
// Copyright (C) 2021 Richard Bradley
//
// fun and useful numeric constants and
// various numeric functions needing a home
//

#pragma once
#include <type_traits>
#include <cmath>


#if defined(__GNUC__) && !defined(__clang__)
#  ifndef __has_builtin
#  define __has_builtin(x) 0
#  endif

#  if !__has_builtin(__builtin_sqrt)
#  error "__builtin_sqrt() required"
#  elif !__has_builtin(__builtin_cos)
#  error "__builtin_cos() required"
#  elif !__has_builtin(__builtin_sin)
#  error "__builtin_sin() required"
#  endif
#endif


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
  [[nodiscard]] constexpr fltType DegToRad(const fltType& deg)
  {
    static_assert(std::is_floating_point_v<fltType>);
    return deg * DEG_TO_RAD<fltType>;
  }

  template<typename fltType>
  [[nodiscard]] constexpr fltType RadToDeg(const fltType& rad)
  {
    static_assert(std::is_floating_point_v<fltType>);
    return rad * RAD_TO_DEG<fltType>;
  }

  template<typename fltType>
  [[nodiscard]] constexpr bool IsZero(const fltType& x)
  {
    if constexpr (std::is_floating_point_v<fltType>) {
      return (x > -VERY_SMALL<fltType>) && (x < VERY_SMALL<fltType>);
    } else {
      return (x == 0);
    }
  }

  template<typename fltType>
  [[nodiscard]] constexpr bool IsOne(const fltType& x)
  {
    if constexpr (std::is_floating_point_v<fltType>) {
      return (x > (static_cast<fltType>(1) - VERY_SMALL<fltType>))
        && (x < (static_cast<fltType>(1) + VERY_SMALL<fltType>));
    } else {
      return (x == 1);
    }
  }

  template<typename fltType>
  [[nodiscard]] constexpr bool IsPos(const fltType& x)
  {
    if constexpr (std::is_floating_point_v<fltType>) {
      return (x >= VERY_SMALL<fltType>);
    } else {
      return (x > 0);
    }
  }

  template<typename fltType>
  [[nodiscard]] constexpr bool IsNeg(const fltType& x)
  {
    if constexpr (std::is_floating_point_v<fltType>) {
      return (x <= -VERY_SMALL<fltType>);
    } else {
      return (x < 0);
    }
  }

  template<typename fltType>
  [[nodiscard]] constexpr bool IsEq(const fltType& x, const fltType& y)
  {
    if constexpr (std::is_floating_point_v<fltType>) {
      return IsZero(x-y);
    } else {
      return (x == y);
    }
  }

  template<typename fltType>
  [[nodiscard]] constexpr bool IsLT(const fltType& x, const fltType& y)
  {
    if constexpr (std::is_floating_point_v<fltType>) {
      return IsNeg(x-y);
    } else {
      return (x < y);
    }
  }

  template<typename fltType>
  [[nodiscard]] constexpr bool IsLTE(const fltType& x, const fltType& y)
  {
    if constexpr (std::is_floating_point_v<fltType>) {
      return !IsPos(x-y);
    } else {
      return (x <= y);
    }
  }

  template<typename fltType>
  [[nodiscard]] constexpr bool IsGT(const fltType& x, const fltType& y)
  {
    if constexpr (std::is_floating_point_v<fltType>) {
      return IsPos(x-y);
    } else {
      return (x > y);
    }
  }

  template<typename fltType>
  [[nodiscard]] constexpr bool IsGTE(const fltType& x, const fltType& y)
  {
    if constexpr (std::is_floating_point_v<fltType>) {
      return !IsNeg(x-y);
    } else {
      return (x >= y);
    }
  }

  template<typename intType>
  [[nodiscard]] constexpr bool IsPowerOf2(const intType& x)
  {
    static_assert(std::is_integral_v<intType>);
    return (x & (x - 1)) == 0;
  }

  template<typename fltType>
  [[nodiscard]] constexpr fltType Lerp(
    const fltType& a, const fltType& b, const fltType& s)
  {
    // use std::lerp() for C++20
    static_assert(std::is_floating_point_v<fltType>);
    if (s <= 0) {
      return a;
    } else if (s >= static_cast<fltType>(1)) {
      return b;
    } else {
      return a + ((b - a) * s);
    }
  }

  template<typename numType>
  [[nodiscard]] constexpr numType Sqr(const numType& x) { return x * x; }

  template<typename numType>
  [[nodiscard]] constexpr numType iPow(numType x, int y)
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
  [[nodiscard]] constexpr int Sgn(const numType& x)
  {
    return (x < 0) ? -1 : ((x > 0) ? 1 : 0);
  }

  template<typename numType>
  [[nodiscard]] constexpr numType Abs(const numType& x)
  {
    // constexpr version of std::abs
    // NOTE: Abs(-MAX_INT) is undefined
    return (x < 0) ? -x : x;
  }

#if defined(__GNUC__) && !defined(__clang__)
  // constexpr versions of sqrt,cos,sin
  [[nodiscard]] constexpr float Sqrt(float x) {
    return __builtin_sqrtf(x); }
  [[nodiscard]] constexpr double Sqrt(double x) {
    return __builtin_sqrt(x); }
  [[nodiscard]] constexpr long double Sqrt(long double x) {
    return __builtin_sqrtl(x); }

  [[nodiscard]] constexpr float Cos(float x) {
    return __builtin_cosf(x); }
  [[nodiscard]] constexpr double Cos(double x) {
    return __builtin_cos(x); }
  [[nodiscard]] constexpr long double Cos(long double x) {
    return __builtin_cosl(x); }

  [[nodiscard]] constexpr float Sin(float x) {
    return __builtin_sinf(x); }
  [[nodiscard]] constexpr double Sin(double x) {
    return __builtin_sin(x); }
  [[nodiscard]] constexpr long double Sin(long double x) {
    return __builtin_sinl(x); }
#else
  // __builtin functions aren't constexpr for clang
  template<typename T>
  [[nodiscard]] T Sqrt(const T& x) { return std::sqrt(x); }
  template<typename T>
  [[nodiscard]] T Cos(const T& x) { return std::cos(x); }
  template<typename T>
  [[nodiscard]] T Sin(const T& x) { return std::sin(x); }
#endif
}
