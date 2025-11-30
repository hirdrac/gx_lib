//
// gx/Vector3D.hh
// Copyright (C) 2025 Richard Bradley
//
// vector template types/functions for 3D calculations
//

#pragma once
#include "MathUtil.hh"
#include "InitType.hh"
#include "Assert.hh"
#include <ostream>


namespace gx {
  // **** Template Types ****
  template<class T> class Vector2;
  template<class T> class Vector3;
  template<class T> class Vector4;
}


template<class T>
class gx::Vector2
{
 public:
  union {
    T _val[2]; // only use [] operator for constexpr access
    struct { T x, y; };
  };

  using type = Vector2<T>;
  using value_type = T;
  using size_type = unsigned int;


  explicit Vector2(NoInit_t) { }
  constexpr Vector2() : _val{} { }
  constexpr Vector2(T vx, T vy) : _val{vx, vy} { }


  // Operators
  [[nodiscard]] constexpr T& operator[](size_type i) {
    return _val[i]; }
  [[nodiscard]] constexpr const T& operator[](size_type i) const {
    return _val[i]; }

  constexpr type& operator+=(const type& v) {
    _val[0] += v[0]; _val[1] += v[1]; return *this; }
  constexpr type& operator-=(const type& v) {
    _val[0] -= v[0]; _val[1] -= v[1]; return *this; }
  constexpr type& operator*=(T v) {
    _val[0] *= v; _val[1] *= v; return *this; }
  constexpr type& operator/=(T v) {
    _val[0] /= v; _val[1] /= v; return *this; }

  [[nodiscard]] constexpr bool operator==(const type& v) const {
    return (_val[0] == v[0]) && (_val[1] == v[1]); }
  [[nodiscard]] constexpr bool operator!=(const type& v) const {
    return (_val[0] != v[0]) || (_val[1] != v[1]); }
  [[nodiscard]] constexpr type operator-() const {
    return {-_val[0], -_val[1]}; }


  // Iterators
  using iterator = T*;
  using const_iterator = const T*;

  [[nodiscard]] constexpr iterator begin() { return data(); }
  [[nodiscard]] constexpr const_iterator begin() const { return data(); }
  [[nodiscard]] constexpr const_iterator cbegin() const { return data(); }

  [[nodiscard]] constexpr iterator end() { return data() + size(); }
  [[nodiscard]] constexpr const_iterator end() const { return data() + size(); }
  [[nodiscard]] constexpr const_iterator cend() const { return data() + size(); }


  // Member Functions
  [[nodiscard]] static constexpr size_type size() { return 2; }
  [[nodiscard]] constexpr T* data() { return _val; }
  [[nodiscard]] constexpr const T* data() const { return _val; }

  [[nodiscard]] constexpr T lengthSqr() const {
    return sqr(_val[0]) + sqr(_val[1]); }
  [[nodiscard]] T length() const { return std::sqrt(lengthSqr()); }
  [[nodiscard]] constexpr bool isUnit() const { return isOne(lengthSqr()); }

  constexpr void set(T vx, T vy) { _val[0] = vx; _val[1] = vy; }
  constexpr void set(const T* v) { set(v[0], v[1]); }

  // Vector2 swizzle
  [[nodiscard]] constexpr Vector2<T> yx() const { return {_val[1], _val[0]}; }
};


template<class T>
class gx::Vector3
{
 public:
  union {
    T _val[3]; // only use [] operator for constexpr access
    struct { T x, y, z; };
    struct { T r, g, b; };
  };

  using type = Vector3<T>;
  using value_type = T;
  using size_type = unsigned int;


  explicit Vector3(NoInit_t) { }
  constexpr Vector3() : _val{} { }
  constexpr Vector3(T vx, T vy, T vz) : _val{vx, vy, vz} { }
  constexpr Vector3(const Vector2<T>& v, T vz)
    : _val{v[0], v[1], vz} { }


  // Operators
  [[nodiscard]] constexpr T& operator[](size_type i) {
    return _val[i]; }
  [[nodiscard]] constexpr const T& operator[](size_type i) const {
    return _val[i]; }

  constexpr type& operator+=(const type& v) {
    _val[0] += v[0]; _val[1] += v[1]; _val[2] += v[2]; return *this; }
  constexpr type& operator-=(const type& v) {
    _val[0] -= v[0]; _val[1] -= v[1]; _val[2] -= v[2]; return *this; }
  constexpr type& operator*=(T v) {
    _val[0] *= v; _val[1] *= v; _val[2] *= v; return *this; }
  constexpr type& operator/=(T v) {
    _val[0] /= v; _val[1] /= v; _val[2] /= v; return *this; }

  [[nodiscard]] constexpr bool operator==(const type& v) const {
    return (_val[0] == v[0]) && (_val[1] == v[1]) && (_val[2] == v[2]); }
  [[nodiscard]] constexpr bool operator!=(const type& v) const {
    return (_val[0] != v[0]) || (_val[1] != v[1]) || (_val[2] != v[2]); }
  [[nodiscard]] constexpr type operator-() const {
    return {-_val[0], -_val[1], -_val[2]}; }


  // Iterators
  using iterator = T*;
  using const_iterator = const T*;

  [[nodiscard]] constexpr iterator begin() { return data(); }
  [[nodiscard]] constexpr const_iterator begin() const { return data(); }
  [[nodiscard]] constexpr const_iterator cbegin() const { return data(); }

  [[nodiscard]] constexpr iterator end() { return data() + size(); }
  [[nodiscard]] constexpr const_iterator end() const { return data() + size(); }
  [[nodiscard]] constexpr const_iterator cend() const { return data() + size(); }


  // Member Functions
  [[nodiscard]] static constexpr size_type size() { return 3; }
  [[nodiscard]] constexpr T* data() { return _val; }
  [[nodiscard]] constexpr const T* data() const { return _val; }

  [[nodiscard]] constexpr T lengthSqr() const {
    return sqr(_val[0]) + sqr(_val[1]) + sqr(_val[2]); }
  [[nodiscard]] T length() const { return std::sqrt(lengthSqr()); }
  [[nodiscard]] constexpr bool isUnit() const { return isOne(lengthSqr()); }

  constexpr void set(T vx, T vy, T vz) {
    _val[0] = vx; _val[1] = vy; _val[2] = vz; }
  constexpr void set(const T* v) { set(v[0], v[1], v[2]); }
  constexpr void set(const Vector2<T>& v, T vz) { set(v[0], v[1], vz); }

  // Vector2 swizzle
  [[nodiscard]] constexpr Vector2<T> xy() const { return {_val[0], _val[1]}; }
  [[nodiscard]] constexpr Vector2<T> xz() const { return {_val[0], _val[2]}; }
  [[nodiscard]] constexpr Vector2<T> yx() const { return {_val[1], _val[0]}; }
  [[nodiscard]] constexpr Vector2<T> yz() const { return {_val[1], _val[2]}; }
  [[nodiscard]] constexpr Vector2<T> zx() const { return {_val[2], _val[0]}; }
  [[nodiscard]] constexpr Vector2<T> zy() const { return {_val[2], _val[1]}; }
};


template<class T>
class gx::Vector4
{
 public:
  union {
    T _val[4]; // only use [] operator for constexpr access
    struct { T x, y, z, w; };
    struct { T r, g, b, a; };
  };

  using type = Vector4<T>;
  using value_type = T;
  using size_type = unsigned int;


  explicit Vector4(NoInit_t) { }
  constexpr Vector4() : _val{} { }
  constexpr Vector4(T vx, T vy, T vz, T vw) : _val{vx, vy, vz, vw} { }
  constexpr Vector4(const Vector3<T>& v, T vw)
    : _val{v[0], v[1], v[2], vw} { }


  // Operators
  [[nodiscard]] constexpr T& operator[](size_type i) {
    return _val[i]; }
  [[nodiscard]] constexpr const T& operator[](size_type i) const {
    return _val[i]; }

  constexpr type& operator+=(const type& v) {
    _val[0] += v[0]; _val[1] += v[1];
    _val[2] += v[2]; _val[3] += v[3]; return *this;
  }
  constexpr type& operator-=(const type& v) {
    _val[0] -= v[0]; _val[1] -= v[1];
    _val[2] -= v[2]; _val[3] -= v[3]; return *this;
  }
  constexpr type& operator*=(T v) {
    _val[0] *= v; _val[1] *= v; _val[2] *= v; _val[3] *= v; return *this; }
  constexpr type& operator/=(T v) {
    _val[0] /= v; _val[1] /= v; _val[2] /= v; _val[3] /= v; return *this; }

  [[nodiscard]] constexpr bool operator==(const type& v) const {
    return (_val[0] == v[0]) && (_val[1] == v[1])
      && (_val[2] == v[2]) && (_val[3] == v[3]);
  }
  [[nodiscard]] constexpr bool operator!=(const type& v) const {
    return (_val[0] != v[0]) || (_val[1] != v[1])
      || (_val[2] != v[2]) || (_val[3] != v[3]);
  }
  [[nodiscard]] constexpr type operator-() const {
    return {-_val[0], -_val[1], -_val[2], -_val[3]}; }


  // Iterators
  using iterator = T*;
  using const_iterator = const T*;

  [[nodiscard]] constexpr iterator begin() { return data(); }
  [[nodiscard]] constexpr const_iterator begin() const { return data(); }
  [[nodiscard]] constexpr const_iterator cbegin() const { return data(); }

  [[nodiscard]] constexpr iterator end() { return data() + size(); }
  [[nodiscard]] constexpr const_iterator end() const { return data() + size(); }
  [[nodiscard]] constexpr const_iterator cend() const { return data() + size(); }


  // Member Functions
  [[nodiscard]] static constexpr size_type size() { return 4; }
  [[nodiscard]] constexpr T* data() { return _val; }
  [[nodiscard]] constexpr const T* data() const { return _val; }

  constexpr void set(T vx, T vy, T vz, T vw) {
    _val[0] = vx; _val[1] = vy; _val[2] = vz; _val[3] = vw; }
  constexpr void set(const T* v) { set(v[0], v[1], v[2], v[3]); }
  constexpr void set(const Vector3<T>& v, T vw) { set(v[0], v[1], v[2], vw); }

  // Vector2 swizzle
  [[nodiscard]] constexpr Vector2<T> xy() const { return {_val[0], _val[1]}; }
  [[nodiscard]] constexpr Vector2<T> xz() const { return {_val[0], _val[2]}; }
  [[nodiscard]] constexpr Vector2<T> xw() const { return {_val[0], _val[3]}; }
  [[nodiscard]] constexpr Vector2<T> yx() const { return {_val[1], _val[0]}; }
  [[nodiscard]] constexpr Vector2<T> yz() const { return {_val[1], _val[2]}; }
  [[nodiscard]] constexpr Vector2<T> yw() const { return {_val[1], _val[3]}; }
  [[nodiscard]] constexpr Vector2<T> zx() const { return {_val[2], _val[0]}; }
  [[nodiscard]] constexpr Vector2<T> zy() const { return {_val[2], _val[1]}; }
  [[nodiscard]] constexpr Vector2<T> zw() const { return {_val[2], _val[3]}; }
  [[nodiscard]] constexpr Vector2<T> wx() const { return {_val[3], _val[0]}; }
  [[nodiscard]] constexpr Vector2<T> wy() const { return {_val[3], _val[1]}; }
  [[nodiscard]] constexpr Vector2<T> wz() const { return {_val[3], _val[2]}; }

  // Vector3 swizzle
  [[nodiscard]] constexpr Vector3<T> xyz() const {
    return {_val[0], _val[1], _val[2]}; }
  [[nodiscard]] constexpr Vector3<T> xyw() const {
    return {_val[0], _val[1], _val[3]}; }
  [[nodiscard]] constexpr Vector3<T> xzw() const {
    return {_val[0], _val[2], _val[3]}; }
  [[nodiscard]] constexpr Vector3<T> yzw() const {
    return {_val[1], _val[2], _val[3]}; }
  [[nodiscard]] constexpr Vector3<T> rgb() const {
    return {_val[0], _val[1], _val[2]}; }
};


namespace gx {
  // **** Binary Operators ****
  // vec + vec
  template<class T>
  [[nodiscard]] constexpr Vector2<T> operator+(
    const Vector2<T>& a, const Vector2<T>& b)
  {
    return {a[0] + b[0], a[1] + b[1]};
  }

  template<class T>
  [[nodiscard]] constexpr Vector3<T> operator+(
    const Vector3<T>& a, const Vector3<T>& b)
  {
    return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
  }

  template<class T>
  [[nodiscard]] constexpr Vector4<T> operator+(
    const Vector4<T>& a, const Vector4<T>& b)
  {
    return {a[0] + b[0], a[1] + b[1], a[2] + b[2], a[3] + b[3]};
  }

  // vec - vec
  template<class T>
  [[nodiscard]] constexpr Vector2<T> operator-(
    const Vector2<T>& a, const Vector2<T>& b)
  {
    return {a[0] - b[0], a[1] - b[1]};
  }

  template<class T>
  [[nodiscard]] constexpr Vector3<T> operator-(
    const Vector3<T>& a, const Vector3<T>& b)
  {
    return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
  }

  template<class T>
  [[nodiscard]] constexpr Vector4<T> operator-(
    const Vector4<T>& a, const Vector4<T>& b)
  {
    return {a[0] - b[0], a[1] - b[1], a[2] - b[2], a[3] - b[3]};
  }

  // vec * vec
  template<class T>
  [[nodiscard]] constexpr Vector2<T> operator*(
    const Vector2<T>& a, const Vector2<T>& b)
  {
    return {a[0] * b[0], a[1] * b[1]};
  }

  template<class T>
  [[nodiscard]] constexpr Vector3<T> operator*(
    const Vector3<T>& a, const Vector3<T>& b)
  {
    return {a[0] * b[0], a[1] * b[1], a[2] * b[2]};
  }

  template<class T>
  [[nodiscard]] constexpr Vector4<T> operator*(
    const Vector4<T>& a, const Vector4<T>& b)
  {
    return {a[0] * b[0], a[1] * b[1], a[2] * b[2], a[3] * b[3]};
  }

  // vec / vec
  template<class T>
  [[nodiscard]] constexpr Vector2<T> operator/(
    const Vector2<T>& a, const Vector2<T>& b)
  {
    return {a[0] / b[0], a[1] / b[1]};
  }

  template<class T>
  [[nodiscard]] constexpr Vector3<T> operator/(
    const Vector3<T>& a, const Vector3<T>& b)
  {
    return {a[0] / b[0], a[1] / b[1], a[2] / b[2]};
  }

  template<class T>
  [[nodiscard]] constexpr Vector4<T> operator/(
    const Vector4<T>& a, const Vector4<T>& b)
  {
    return {a[0] / b[0], a[1] / b[1], a[2] / b[2], a[3] / b[3]};
  }

  // vec * x
  template<class T>
  [[nodiscard]] constexpr Vector2<T> operator*(const Vector2<T>& a, T b)
  {
    return {a[0] * b, a[1] * b};
  }

  template<class T>
  [[nodiscard]] constexpr Vector3<T> operator*(const Vector3<T>& a, T b)
  {
    return {a[0] * b, a[1] * b, a[2] * b};
  }

  template<class T>
  [[nodiscard]] constexpr Vector4<T> operator*(const Vector4<T>& a, T b)
  {
    return {a[0] * b, a[1] * b, a[2] * b, a[3] * b};
  }

  // x * vec
  template<class T>
  [[nodiscard]] constexpr Vector2<T> operator*(T a, const Vector2<T>& b)
  {
    return {a * b[0], a * b[1]};
  }

  template<class T>
  [[nodiscard]] constexpr Vector3<T> operator*(T a, const Vector3<T>& b)
  {
    return {a * b[0], a * b[1], a * b[2]};
  }

  template<class T>
  [[nodiscard]] constexpr Vector4<T> operator*(T a, const Vector4<T>& b)
  {
    return {a * b[0], a * b[1], a * b[2], a * b[3]};
  }

  // vec / x
  template<class T>
  [[nodiscard]] constexpr Vector2<T> operator/(const Vector2<T>& a, T b)
  {
    return {a[0] / b, a[1] / b};
  }

  template<class T>
  [[nodiscard]] constexpr Vector3<T> operator/(const Vector3<T>& a, T b)
  {
    return {a[0] / b, a[1] / b, a[2] / b};
  }

  template<class T>
  [[nodiscard]] constexpr Vector4<T> operator/(const Vector4<T>& a, T b)
  {
    return {a[0] / b, a[1] / b, a[2] / b, a[3] / b};
  }

  // x / vec
  template<class T>
  [[nodiscard]] constexpr Vector2<T> operator/(T a, const Vector2<T>& b)
  {
    return {a / b[0], a / b[1]};
  }

  template<class T>
  [[nodiscard]] constexpr Vector3<T> operator/(T a, const Vector3<T>& b)
  {
    return {a / b[0], a / b[1], a / b[2]};
  }

  template<class T>
  [[nodiscard]] constexpr Vector4<T> operator/(T a, const Vector4<T>& b)
  {
    return {a / b[0], a / b[1], a / b[2], a / b[3]};
  }


  // **** Template Stream Operators ****
  template<class T>
  std::ostream& operator<<(std::ostream& os, const Vector2<T>& v)
  {
    return os << '[' << v[0] << ' ' << v[1] << ']';
  }

  template<class T>
  std::ostream& operator<<(std::ostream& os, const Vector3<T>& v)
  {
    return os << '[' << v[0] << ' ' << v[1] << ' ' << v[2] << ']';
  }

  template<class T>
  std::ostream& operator<<(std::ostream& os, const Vector4<T>& v)
  {
    return os << '[' << v[0] << ' ' << v[1] << ' '
              << v[2] << ' ' << v[3] << ']';
  }


  // **** Template Functions ****
  template<class T>
  [[nodiscard]] constexpr T pointDistanceSqr(
    const Vector2<T>& a, const Vector2<T>& b)
  {
    return (a - b).lengthSqr();
  }

  template<class T>
  [[nodiscard]] constexpr T pointDistanceSqr(
    const Vector3<T>& a, const Vector3<T>& b)
  {
    return (a - b).lengthSqr();
  }

  template<class T>
  [[nodiscard]] T pointDistance(const Vector2<T>& a, const Vector2<T>& b)
  {
    return (a - b).length();
  }

  template<class T>
  [[nodiscard]] T pointDistance(const Vector3<T>& a, const Vector3<T>& b)
  {
    return (a - b).length();
  }

  template<class T>
  [[nodiscard]] constexpr T dotProduct(
    const Vector2<T>& a, const Vector2<T>& b)
  {
    return (a[0] * b[0]) + (a[1] * b[1]);
  }

  template<class T>
  [[nodiscard]] constexpr T dotProduct(
    const Vector3<T>& a, const Vector3<T>& b)
  {
    return (a[0] * b[0]) + (a[1] * b[1]) + (a[2] * b[2]);
  }

  template<class T>
  [[nodiscard]] constexpr Vector3<T> crossProduct(
    const Vector3<T>& a, const Vector3<T>& b)
  {
    return {(a[1] * b[2]) - (a[2] * b[1]),
            (a[2] * b[0]) - (a[0] * b[2]),
            (a[0] * b[1]) - (a[1] * b[0])};
  }

  template<class T>
  [[nodiscard]] Vector2<T> unitVec(const Vector2<T>& v)
  {
    const T len = v.length();
    GX_ASSERT_DEBUG(isPos(len));
    return v * (T{1} / len);
  }

  template<class T>
  [[nodiscard]] Vector3<T> unitVec(const Vector3<T>& v)
  {
    const T len = v.length();
    GX_ASSERT_DEBUG(isPos(len));
    return v * (T{1} / len);
  }

  template<class T>
  [[nodiscard]] Vector2<T> unitVec(T x, T y)
  {
    const T len = std::sqrt(sqr(x) + sqr(y));
    GX_ASSERT_DEBUG(isPos(len));
    const T inv = T{1} / len;
    return {x * inv, y * inv};
  }

  template<class T>
  [[nodiscard]] Vector3<T> unitVec(T x, T y, T z)
  {
    const T len = std::sqrt(sqr(x) + sqr(y) + sqr(z));
    GX_ASSERT_DEBUG(isPos(len));
    const T inv = T{1} / len;
    return {x * inv, y * inv, z * inv};
  }


  // Simplified vector rotation functions to avoid using a matrix
  template<class T>
  [[nodiscard]] Vector2<T> rotate(const Vector2<T>& v, T rad)
  {
    const T c = std::cos(rad), s = std::sin(rad);
    return {(v.x * c) - (v.y * s), (v.x * s) + (v.y * c)};
  }

  template<class T>
  [[nodiscard]] Vector3<T> rotateX(const Vector3<T>& v, T rad)
  {
    const T c = std::cos(rad), s = std::sin(rad);
    return {v.x, (v.y * c) - (v.z * s), (v.y * s) + (v.z * c)};
  }

  template<class T>
  [[nodiscard]] Vector3<T> rotateY(const Vector3<T>& v, T rad)
  {
    const T c = std::cos(rad), s = std::sin(rad);
    return {(v.x * c) + (v.z * s), v.y, (v.z * c) - (v.x * s)};
  }

  template<class T>
  [[nodiscard]] Vector3<T> rotateZ(const Vector3<T>& v, T rad)
  {
    const T c = std::cos(rad), s = std::sin(rad);
    return {(v.x * c) - (v.y * s), (v.x * s) + (v.y * c), v.z};
  }
}
