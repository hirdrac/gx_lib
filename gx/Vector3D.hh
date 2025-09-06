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
    struct { T x, y; }; // only use x,y for constexpr access
    T _val[2];
  };

  using type = Vector2<T>;
  using value_type = T;
  using size_type = unsigned int;


  explicit Vector2(NoInit_t) { }
  constexpr Vector2() : Vector2{0,0} { }
  constexpr Vector2(T vx, T vy) : x{vx}, y{vy} { }


  // Operators
  [[nodiscard]] constexpr T& operator[](size_type i) { return _val[i]; }
  [[nodiscard]] constexpr const T& operator[](size_type i) const {
    return _val[i]; }

  constexpr type& operator+=(const type& v) {
    x += v.x; y += v.y; return *this; }
  constexpr type& operator-=(const type& v) {
    x -= v.x; y -= v.y; return *this; }
  constexpr type& operator*=(T v) {
    x *= v; y *= v; return *this; }
  constexpr type& operator/=(T v) {
    x /= v; y /= v; return *this; }

  [[nodiscard]] constexpr bool operator==(const type& v) const {
    return (x == v.x) && (y == v.y); }
  [[nodiscard]] constexpr bool operator!=(const type& v) const {
    return (x != v.x) || (y != v.y); }
  [[nodiscard]] constexpr type operator-() const { return {-x, -y}; }


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

  [[nodiscard]] constexpr T lengthSqr() const { return sqr(x)+sqr(y); }
  [[nodiscard]] T length() const { return std::sqrt(lengthSqr()); }
  [[nodiscard]] constexpr bool isUnit() const { return isOne(lengthSqr()); }

  constexpr void set(T vx, T vy) { x = vx; y = vy; }
  constexpr void set(const T* v) { set(v[0], v[1]); }

  // Vector2 swizzle
  [[nodiscard]] constexpr Vector2<T> yx() const { return {y, x}; }
};


template<class T>
class gx::Vector3
{
 public:
  union {
    struct { T x, y, z; }; // only use x,y,z for constexpr access
    struct { T r, g, b; };
    T _val[3];
  };

  using type = Vector3<T>;
  using value_type = T;
  using size_type = unsigned int;


  explicit Vector3(NoInit_t) { }
  constexpr Vector3() : Vector3{0,0,0} { }
  constexpr Vector3(T vx, T vy, T vz) : x{vx}, y{vy}, z{vz} { }
  constexpr Vector3(const Vector2<T>& v, T vz)
    : Vector3{v.x, v.y, vz} { }


  // Operators
  [[nodiscard]] constexpr T& operator[](size_type i) { return _val[i]; }
  [[nodiscard]] constexpr const T& operator[](size_type i) const {
    return _val[i]; }

  constexpr type& operator+=(const type& v) {
    x += v.x; y += v.y; z += v.z; return *this; }
  constexpr type& operator-=(const type& v) {
    x -= v.x; y -= v.y; z -= v.z; return *this; }
  constexpr type& operator*=(T v) {
    x *= v; y *= v; z *= v; return *this; }
  constexpr type& operator/=(T v) {
    x /= v; y /= v; z /= v; return *this; }

  [[nodiscard]] constexpr bool operator==(const type& v) const {
    return (x == v.x) && (y == v.y) && (z == v.z); }
  [[nodiscard]] constexpr bool operator!=(const type& v) const {
    return (x != v.x) || (y != v.y) || (z != v.z); }
  [[nodiscard]] constexpr type operator-() const { return {-x, -y, -z}; }


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

  [[nodiscard]] constexpr T lengthSqr() const { return sqr(x)+sqr(y)+sqr(z); }
  [[nodiscard]] T length() const { return std::sqrt(lengthSqr()); }
  [[nodiscard]] constexpr bool isUnit() const { return isOne(lengthSqr()); }

  constexpr void set(T vx, T vy, T vz) { x = vx; y = vy; z = vz; }
  constexpr void set(const T* v) { set(v[0], v[1], v[2]); }
  constexpr void set(Vector2<T> v, T vz) { set(v.x, v.y, vz); }

  // Vector2 swizzle
  [[nodiscard]] constexpr Vector2<T> xy() const { return {x, y}; }
  [[nodiscard]] constexpr Vector2<T> xz() const { return {x, z}; }
  [[nodiscard]] constexpr Vector2<T> yx() const { return {y, x}; }
  [[nodiscard]] constexpr Vector2<T> yz() const { return {y, z}; }
  [[nodiscard]] constexpr Vector2<T> zx() const { return {z, x}; }
  [[nodiscard]] constexpr Vector2<T> zy() const { return {z, y}; }
};


template<class T>
class gx::Vector4
{
 public:
  union {
    struct { T x, y, z, w; }; // only use x,y,z,w for constexpr access
    struct { T r, g, b, a; };
    T _val[4];
  };

  using type = Vector4<T>;
  using value_type = T;
  using size_type = unsigned int;


  explicit Vector4(NoInit_t) { }
  constexpr Vector4() : Vector4{0,0,0,0} { }
  constexpr Vector4(T vx, T vy, T vz, T vw) : x{vx}, y{vy}, z{vz}, w{vw} { }
  constexpr Vector4(const Vector3<T>& v, T vw)
    : Vector4{v.x, v.y, v.z, vw} { }


  // Operators
  [[nodiscard]] constexpr T& operator[](size_type i) { return _val[i]; }
  [[nodiscard]] constexpr const T& operator[](size_type i) const {
    return _val[i]; }

  constexpr type& operator+=(const type& v) {
    x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
  constexpr type& operator-=(const type& v) {
    x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
  constexpr type& operator*=(T v) {
    x *= v; y *= v; z *= v; w *= v; return *this; }
  constexpr type& operator/=(T v) {
    x /= v; y /= v; z /= v; w /= v; return *this; }

  [[nodiscard]] constexpr bool operator==(const type& v) const {
    return (x == v.x) && (y == v.y) && (z == v.z) && (w == v.w); }
  [[nodiscard]] constexpr bool operator!=(const type& v) const {
    return (x != v.x) || (y != v.y) || (z != v.z) || (w != v.w); }
  [[nodiscard]] constexpr type operator-() const { return {-x, -y, -z, -w}; }


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

  constexpr void set(T vx, T vy, T vz, T vw) { x = vx; y = vy; z = vz; w = vw; }
  constexpr void set(const T* v) { set(v[0], v[1], v[2], v[3]); }
  constexpr void set(const Vector3<T>& v, T vw) { set(v.x, v.y, v.z, vw); }

  // Vector2 swizzle
  [[nodiscard]] constexpr Vector2<T> xy() const { return {x, y}; }
  [[nodiscard]] constexpr Vector2<T> xz() const { return {x, z}; }
  [[nodiscard]] constexpr Vector2<T> xw() const { return {x, w}; }
  [[nodiscard]] constexpr Vector2<T> yx() const { return {y, x}; }
  [[nodiscard]] constexpr Vector2<T> yz() const { return {y, z}; }
  [[nodiscard]] constexpr Vector2<T> yw() const { return {y, w}; }
  [[nodiscard]] constexpr Vector2<T> zx() const { return {z, x}; }
  [[nodiscard]] constexpr Vector2<T> zy() const { return {z, y}; }
  [[nodiscard]] constexpr Vector2<T> zw() const { return {z, w}; }
  [[nodiscard]] constexpr Vector2<T> wx() const { return {w, x}; }
  [[nodiscard]] constexpr Vector2<T> wy() const { return {w, y}; }
  [[nodiscard]] constexpr Vector2<T> wz() const { return {w, z}; }

  // Vector3 swizzle
  [[nodiscard]] constexpr Vector3<T> xyz() const { return {x, y, z}; }
  [[nodiscard]] constexpr Vector3<T> xyw() const { return {x, y, w}; }
  [[nodiscard]] constexpr Vector3<T> xzw() const { return {x, z, w}; }
  [[nodiscard]] constexpr Vector3<T> yzw() const { return {y, z, w}; }
  [[nodiscard]] constexpr Vector3<T> rgb() const { return {r, g, b}; }
};


namespace gx {
  // **** Binary Operators ****
  // vec + vec
  template<class T>
  [[nodiscard]] constexpr Vector2<T> operator+(const Vector2<T>& a, const Vector2<T>& b) {
    return {a.x + b.x, a.y + b.y}; }

  template<class T>
  [[nodiscard]] constexpr Vector3<T> operator+(const Vector3<T>& a, const Vector3<T>& b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z}; }

  template<class T>
  [[nodiscard]] constexpr Vector4<T> operator+(const Vector4<T>& a, const Vector4<T>& b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w}; }

  // vec - vec
  template<class T>
  [[nodiscard]] constexpr Vector2<T> operator-(const Vector2<T>& a, const Vector2<T>& b) {
    return {a.x - b.x, a.y - b.y}; }

  template<class T>
  [[nodiscard]] constexpr Vector3<T> operator-(const Vector3<T>& a, const Vector3<T>& b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z}; }

  template<class T>
  [[nodiscard]] constexpr Vector4<T> operator-(const Vector4<T>& a, const Vector4<T>& b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w}; }

  // vec * vec
  template<class T>
  [[nodiscard]] constexpr Vector2<T> operator*(const Vector2<T>& a, const Vector2<T>& b) {
    return {a.x * b.x, a.y * b.y}; }

  template<class T>
  [[nodiscard]] constexpr Vector3<T> operator*(const Vector3<T>& a, const Vector3<T>& b) {
    return {a.x * b.x, a.y * b.y, a.z * b.z}; }

  template<class T>
  [[nodiscard]] constexpr Vector4<T> operator*(const Vector4<T>& a, const Vector4<T>& b) {
    return {a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w}; }

  // vec / vec
  template<class T>
  [[nodiscard]] constexpr Vector2<T> operator/(const Vector2<T>& a, const Vector2<T>& b) {
    return {a.x / b.x, a.y / b.y}; }

  template<class T>
  [[nodiscard]] constexpr Vector3<T> operator/(const Vector3<T>& a, const Vector3<T>& b) {
    return {a.x / b.x, a.y / b.y, a.z / b.z}; }

  template<class T>
  [[nodiscard]] constexpr Vector4<T> operator/(const Vector4<T>& a, const Vector4<T>& b) {
    return {a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w}; }

  // vec * x
  template<class T>
  [[nodiscard]] constexpr Vector2<T> operator*(const Vector2<T>& a, T b) {
    return {a.x * b, a.y * b}; }

  template<class T>
  [[nodiscard]] constexpr Vector3<T> operator*(const Vector3<T>& a, T b) {
    return {a.x * b, a.y * b, a.z * b}; }

  template<class T>
  [[nodiscard]] constexpr Vector4<T> operator*(const Vector4<T>& a, T b) {
    return {a.x * b, a.y * b, a.z * b, a.w * b}; }

  // x * vec
  template<class T>
  [[nodiscard]] constexpr Vector2<T> operator*(T a, const Vector2<T>& b) {
    return {a * b.x, a * b.y}; }

  template<class T>
  [[nodiscard]] constexpr Vector3<T> operator*(T a, const Vector3<T>& b) {
    return {a * b.x, a * b.y, a * b.z}; }

  template<class T>
  [[nodiscard]] constexpr Vector4<T> operator*(T a, const Vector4<T>& b) {
    return {a * b.x, a * b.y, a * b.z, a * b.w}; }

  // vec / x
  template<class T>
  [[nodiscard]] constexpr Vector2<T> operator/(const Vector2<T>& a, T b) {
    return {a.x / b, a.y / b}; }

  template<class T>
  [[nodiscard]] constexpr Vector3<T> operator/(const Vector3<T>& a, T b) {
    return {a.x / b, a.y / b, a.z / b}; }

  template<class T>
  [[nodiscard]] constexpr Vector4<T> operator/(const Vector4<T>& a, T b) {
    return {a.x / b, a.y / b, a.z / b, a.w / b}; }

  // x / vec
  template<class T>
  [[nodiscard]] constexpr Vector2<T> operator/(T a, const Vector2<T>& b) {
    return {a / b.x, a / b.y}; }

  template<class T>
  [[nodiscard]] constexpr Vector3<T> operator/(T a, const Vector3<T>& b) {
    return {a / b.x, a / b.y, a / b.z}; }

  template<class T>
  [[nodiscard]] constexpr Vector4<T> operator/(T a, const Vector4<T>& b) {
    return {a / b.x, a / b.y, a / b.z, a / b.w}; }


  // **** Template Stream Operators ****
  template<class T>
  std::ostream& operator<<(std::ostream& os, const Vector2<T>& v) {
    return os << '[' << v.x << ' ' << v.y << ']'; }

  template<class T>
  std::ostream& operator<<(std::ostream& os, const Vector3<T>& v) {
    return os << '[' << v.x << ' ' << v.y << ' ' << v.z << ']'; }

  template<class T>
  std::ostream& operator<<(std::ostream& os, const Vector4<T>& v) {
    return os << '[' << v.x << ' ' << v.y << ' ' << v.z << ' ' << v.w << ']'; }


  // **** Template Functions ****
  template<class T>
  [[nodiscard]] constexpr T pointDistanceSqr(const Vector2<T>& a, const Vector2<T>& b) {
    return (a - b).lengthSqr(); }

  template<class T>
  [[nodiscard]] constexpr T pointDistanceSqr(const Vector3<T>& a, const Vector3<T>& b) {
    return (a - b).lengthSqr(); }

  template<class T>
  [[nodiscard]] T pointDistance(const Vector2<T>& a, const Vector2<T>& b) {
    return (a - b).length(); }

  template<class T>
  [[nodiscard]] T pointDistance(const Vector3<T>& a, const Vector3<T>& b) {
    return (a - b).length(); }

  template<class T>
  [[nodiscard]] constexpr T dotProduct(const Vector2<T>& a, const Vector2<T>& b) {
    return (a.x * b.x) + (a.y * b.y); }

  template<class T>
  [[nodiscard]] constexpr T dotProduct(const Vector3<T>& a, const Vector3<T>& b) {
    return (a.x * b.x) + (a.y * b.y) + (a.z * b.z); }

  template<class T>
  [[nodiscard]] constexpr Vector3<T> crossProduct(const Vector3<T>& a, const Vector3<T>& b)
  {
    return {(a.y * b.z) - (a.z * b.y),
            (a.z * b.x) - (a.x * b.z),
            (a.x * b.y) - (a.y * b.x)};
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
