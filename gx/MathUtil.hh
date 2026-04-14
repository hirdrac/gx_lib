//
// gx/MathUtil.hh
// Copyright (C) 2026 Richard Bradley
//
// useful numeric constants and various numeric functions
//

#pragma once
#include <type_traits>
#include <concepts>
#include <numbers>

// basic type assumptions
static_assert(sizeof(char) == 1);
static_assert(sizeof(short) == 2);
static_assert(sizeof(int) == 4);


namespace gx {
  // **** Concepts ****
  template<class T>
  concept NumType = std::integral<T> || std::floating_point<T>;


  // **** Constants ****
  template<std::floating_point T>
  constexpr T PI = std::numbers::pi_v<T>;

  template<std::floating_point T>
  constexpr T DEG_TO_RAD = PI<T> / T{180};

  template<std::floating_point T>
  constexpr T RAD_TO_DEG = T{180} / PI<T>;

  template<std::floating_point T>
  constexpr T VERY_SMALL = static_cast<T>(1.0e-12);
  template<>
  inline constexpr float VERY_SMALL<float> = 1.0e-7f;


  // **** Functions ****
  template<std::floating_point T>
  [[nodiscard]] constexpr T degToRad(T deg) {
    return deg * DEG_TO_RAD<T>;
  }

  template<std::floating_point T>
  [[nodiscard]] constexpr T radToDeg(T rad) {
    return rad * RAD_TO_DEG<T>;
  }

  template<std::floating_point T>
  [[nodiscard]] constexpr bool isZero(T x) {
    return (x > -VERY_SMALL<T>) && (x < VERY_SMALL<T>);
  }

  template<std::integral T>
  [[nodiscard]] constexpr bool isZero(T x) {
    return x == 0;
  }

  template<std::floating_point T>
  [[nodiscard]] constexpr bool isOne(T x) {
    return (x > (T{1} - VERY_SMALL<T>)) && (x < (T{1} + VERY_SMALL<T>));
  }

  template<std::integral T>
  [[nodiscard]] constexpr bool isOne(T x) {
    return x == 1;
  }

  template<std::floating_point T>
  [[nodiscard]] constexpr bool isPos(T x) {
    return (x >= VERY_SMALL<T>);
  }

  template<std::integral T>
  [[nodiscard]] constexpr bool isPos(T x) {
    return x > 0;
  }

  template<std::floating_point T>
  [[nodiscard]] constexpr bool isNeg(T x) {
    return x <= -VERY_SMALL<T>;
  }

  template<std::integral T>
  [[nodiscard]] constexpr bool isNeg(T x) {
    return x < 0;
  }

  template<std::floating_point T>
  [[nodiscard]] constexpr bool isEq(T x, T y) {
    return isZero(x-y);
  }

  template<std::integral T>
  [[nodiscard]] constexpr bool isEq(T x, T y) {
    return x == y;
  }

  template<NumType T>
  [[nodiscard]] constexpr bool isLT(T x, T y) {
    return isNeg(x-y);
  }

  template<NumType T>
  [[nodiscard]] constexpr bool isLTE(T x, T y) {
    return !isPos(x-y);
  }

  template<NumType T>
  [[nodiscard]] constexpr bool isGT(T x, T y) {
    return isPos(x-y);
  }

  template<NumType T>
  [[nodiscard]] constexpr bool isGTE(T x, T y) {
    return !isNeg(x-y);
  }

  template<std::integral T>
  [[nodiscard]] constexpr bool isPowerOf2(T x) {
    return (x & (x - 1)) == 0;
  }

  template<std::floating_point T>
  [[nodiscard]] constexpr T lerp(T a, T b, T x)
  {
    if (x <= 0) {
      return a;
    } else if (x >= T{1}) {
      return b;
    } else {
      return a + ((b - a) * x);
    }
  }

  template<NumType T>
  [[nodiscard]] constexpr T sqr(T x) { return x * x; }

  template<NumType T>
  [[nodiscard]] constexpr T ipow(T x, int y)
  {
    if (y < 0) { return 0; }

    T val = 1;
    while (y) {
      if (y & 1) { val *= x; }
      y >>= 1;
      x *= x;
    }

    return val;
  }

  template<NumType T>
  [[nodiscard]] constexpr int sign(T x)
  {
    if constexpr (std::is_signed_v<T>) {
      return int{isPos(x)} - int{isNeg(x)};
    } else {
      return int{isPos(x)};
    }
  }

  // constexpr version of std::abs
  //   return type promoted to int for small types
  [[nodiscard]] constexpr int abs(signed char x) {
    return (x < 0) ? -int{x} : int{x}; }
  [[nodiscard]] constexpr int abs(short x) {
    return (x < 0) ? -int{x} : int{x}; }

  // template for all other types
  template<NumType T>
  [[nodiscard]] constexpr T abs(T x)
  {
    if constexpr (std::is_signed_v<T>) {
      // NOTE: abs(-MAX_INT) is undefined
      return (x < 0) ? -x : x;
    } else {
      return x;
    }
  }
}
