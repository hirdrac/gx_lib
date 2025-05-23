//
// gx/MathUtil.hh
// Copyright (C) 2025 Richard Bradley
//
// useful numeric constants and various numeric functions
//

#pragma once
#include <type_traits>
#include <cmath>

// basic type assumptions
static_assert(sizeof(char) == 1);
static_assert(sizeof(short) == 2);
static_assert(sizeof(int) == 4);


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
  constexpr T DEG_TO_RAD = PI<T> / T{180};

  template<class T>
  constexpr T RAD_TO_DEG = T{180} / PI<T>;

  template<class T>
  constexpr T VERY_SMALL = static_cast<T>(1.0e-12);

  template<>
  inline constexpr float VERY_SMALL<float> = 1.0e-7f;


  // **** Functions ****
  template<class fltType>
  [[nodiscard]] constexpr fltType degToRad(fltType deg)
  {
    static_assert(std::is_floating_point_v<fltType>);
    return deg * DEG_TO_RAD<fltType>;
  }

  template<class fltType>
  [[nodiscard]] constexpr fltType radToDeg(fltType rad)
  {
    static_assert(std::is_floating_point_v<fltType>);
    return rad * RAD_TO_DEG<fltType>;
  }

  template<class numType>
  [[nodiscard]] constexpr bool isZero(numType x)
  {
    if constexpr (std::is_floating_point_v<numType>) {
      return (x > -VERY_SMALL<numType>) && (x < VERY_SMALL<numType>);
    } else {
      return (x == 0);
    }
  }

  template<class numType>
  [[nodiscard]] constexpr bool isOne(numType x)
  {
    if constexpr (std::is_floating_point_v<numType>) {
      return (x > (numType{1} - VERY_SMALL<numType>))
        && (x < (numType{1} + VERY_SMALL<numType>));
    } else {
      return (x == 1);
    }
  }

  template<class numType>
  [[nodiscard]] constexpr bool isPos(numType x)
  {
    if constexpr (std::is_floating_point_v<numType>) {
      return (x >= VERY_SMALL<numType>);
    } else {
      return (x > 0);
    }
  }

  template<class numType>
  [[nodiscard]] constexpr bool isNeg(numType x)
  {
    if constexpr (std::is_floating_point_v<numType>) {
      return (x <= -VERY_SMALL<numType>);
    } else {
      return (x < 0);
    }
  }

  template<class numType>
  [[nodiscard]] constexpr bool isEq(numType x, numType y)
  {
    if constexpr (std::is_floating_point_v<numType>) {
      return isZero(x-y);
    } else {
      return (x == y);
    }
  }

  template<class numType>
  [[nodiscard]] constexpr bool isLT(numType x, numType y)
  {
    if constexpr (std::is_floating_point_v<numType>) {
      return isNeg(x-y);
    } else {
      return (x < y);
    }
  }

  template<class numType>
  [[nodiscard]] constexpr bool isLTE(numType x, numType y)
  {
    if constexpr (std::is_floating_point_v<numType>) {
      return !isPos(x-y);
    } else {
      return (x <= y);
    }
  }

  template<class numType>
  [[nodiscard]] constexpr bool isGT(numType x, numType y)
  {
    if constexpr (std::is_floating_point_v<numType>) {
      return isPos(x-y);
    } else {
      return (x > y);
    }
  }

  template<class numType>
  [[nodiscard]] constexpr bool isGTE(numType x, numType y)
  {
    if constexpr (std::is_floating_point_v<numType>) {
      return !isNeg(x-y);
    } else {
      return (x >= y);
    }
  }

  template<class intType>
  [[nodiscard]] constexpr bool isPowerOf2(intType x)
  {
    static_assert(std::is_integral_v<intType>);
    return (x & (x - 1)) == 0;
  }

  template<class fltType>
  [[nodiscard]] constexpr fltType lerp(fltType a, fltType b, fltType s)
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

  template<class numType>
  [[nodiscard]] constexpr numType sqr(numType x) { return x * x; }

  template<class numType>
  [[nodiscard]] constexpr numType ipow(numType x, int y)
  {
    if (y < 0) { return 0; }

    numType val = static_cast<numType>(1);
    while (y) {
      if (y & 1) { val *= x; }
      y >>= 1;
      x *= x;
    }

    return val;
  }

  template<class numType>
  [[nodiscard]] constexpr int sgn(numType x)
  {
    if constexpr (std::is_signed_v<numType>) {
      return int{x > 0} - int{x < 0};
    } else {
      return int{x > 0};
    }
  }

  // constexpr version of std::abs
  //   return type promoted to int for small types
  [[nodiscard]] constexpr int abs(signed char x) {
    return (x < 0) ? -int{x} : int{x}; }
  [[nodiscard]] constexpr int abs(short x) {
    return (x < 0) ? -int{x} : int{x}; }

  //   template for all other types
  template<class numType>
  [[nodiscard]] constexpr numType abs(numType x)
  {
    if constexpr (std::is_signed_v<numType>) {
      // NOTE: abs(-MAX_INT) is undefined
      return (x < 0) ? -x : x;
    } else {
      return x;
    }
  }
}
