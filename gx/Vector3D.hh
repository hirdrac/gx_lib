//
// gx/Vector3D.hh
// Copyright (C) 2021 Richard Bradley
//
// vector template types/functions for 3D calculations
//

#pragma once
#include "MathUtil.hh"
#include <ostream>


namespace gx {
  // **** Template Types ****
  template<typename T> class Vector2;
  template<typename T> class Vector3;
  template<typename T> class Vector4;
}


template<typename T>
class gx::Vector2
{
 public:
  union {
    struct { T x, y; };
    T _val[2];
  };

  using type = Vector2<T>;
  using value_type = T;
  using size_type = unsigned int;


  // Constructors
  Vector2() = default;
  constexpr Vector2(T vx, T vy) : x(vx), y(vy) { }


  // Operators
  [[nodiscard]] constexpr T& operator[](size_type i) { return _val[i]; }
  [[nodiscard]] constexpr const T& operator[](size_type i) const { return _val[i]; }

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


  // Iterators
  using iterator = T*;
  using const_iterator = const T*;

  [[nodiscard]] constexpr iterator begin() noexcept { return data(); }
  [[nodiscard]] constexpr const_iterator begin() const noexcept { return data(); }
  [[nodiscard]] constexpr const_iterator cbegin() const noexcept { return data(); }

  [[nodiscard]] constexpr iterator end() noexcept { return data() + size(); }
  [[nodiscard]] constexpr const_iterator end() const noexcept { return data() + size(); }
  [[nodiscard]] constexpr const_iterator cend() const noexcept { return data() + size(); }


  // Member Functions
  [[nodiscard]] static constexpr size_type size() noexcept { return 2; }
  [[nodiscard]] constexpr T* data() noexcept { return _val; }
  [[nodiscard]] constexpr const T* data() const noexcept { return _val; }

  [[nodiscard]] constexpr T lengthSqr() const { return Sqr(x)+Sqr(y); }
  [[nodiscard]] T length() const { return std::sqrt(lengthSqr()); }
  [[nodiscard]] constexpr bool isUnit() const { return IsOne(lengthSqr()); }

  constexpr void set(T vx, T vy) { x = vx; y = vy; }
  constexpr void set(const T* v) { set(v[0], v[1]); }
  constexpr void invert() { x = -x; y = -y; }
  constexpr void normalize() { operator*=(static_cast<T>(1) / length()); }
};


template<typename T>
class gx::Vector3
{
 public:
  union {
    struct { T x, y, z; };
    struct { T r, g, b; };
    T _val[3];
  };

  using type = Vector3<T>;
  using value_type = T;
  using size_type = unsigned int;


  // Constructors
  Vector3() = default;
  constexpr Vector3(T vx, T vy, T vz) : x(vx), y(vy), z(vz) { }


  // Operators
  [[nodiscard]] constexpr T& operator[](size_type i) { return _val[i]; }
  [[nodiscard]] constexpr const T& operator[](size_type i) const { return _val[i]; }

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


  // Iterators
  using iterator = T*;
  using const_iterator = const T*;

  [[nodiscard]] constexpr iterator begin() noexcept { return data(); }
  [[nodiscard]] constexpr const_iterator begin() const noexcept { return data(); }
  [[nodiscard]] constexpr const_iterator cbegin() const noexcept { return data(); }

  [[nodiscard]] constexpr iterator end() noexcept { return data() + size(); }
  [[nodiscard]] constexpr const_iterator end() const noexcept { return data() + size(); }
  [[nodiscard]] constexpr const_iterator cend() const noexcept { return data() + size(); }


  // Member Functions
  [[nodiscard]] static constexpr size_type size() noexcept { return 3; }
  [[nodiscard]] constexpr T* data() noexcept { return _val; }
  [[nodiscard]] constexpr const T* data() const noexcept { return _val; }

  [[nodiscard]] constexpr T lengthSqr() const { return Sqr(x)+Sqr(y)+Sqr(z); }
  [[nodiscard]] T length() const { return std::sqrt(lengthSqr()); }
  [[nodiscard]] constexpr bool isUnit() const { return IsOne(lengthSqr()); }

  constexpr void set(T vx, T vy, T vz) { x = vx; y = vy; z = vz; }
  constexpr void set(const T* v) { set(v[0], v[1], v[2]); }
  constexpr void invert() { x = -x; y = -y; z = -z; }
  constexpr void normalize() { operator*=(static_cast<T>(1) / length()); }

  // Vector2 swizzle
  [[nodiscard]] constexpr Vector2<T>& xy() {
    return *reinterpret_cast<Vector2<T>*>(data()); }
  [[nodiscard]] constexpr const Vector2<T>& xy() const {
    return *reinterpret_cast<const Vector2<T>*>(data()); }

  [[nodiscard]] constexpr Vector2<T>& yz() {
    return *reinterpret_cast<Vector2<T>*>(data()+1); }
  [[nodiscard]] constexpr const Vector2<T>& yz() const {
    return *reinterpret_cast<const Vector2<T>*>(data()+1); }
};


template<typename T>
class gx::Vector4
{
 public:
  union {
    struct { T x, y, z, w; };
    struct { T r, g, b, a; };
    T _val[4];
  };

  using type = Vector4<T>;
  using value_type = T;
  using size_type = unsigned int;


  // Constructors
  Vector4() = default;
  constexpr Vector4(T vx, T vy, T vz, T vw) : x(vx), y(vy), z(vz), w(vw) { }
  constexpr Vector4(const Vector3<T>& v, T vw)
    : Vector4(v[0], v[1], v[2], vw) { }


  // Operators
  [[nodiscard]] constexpr T& operator[](size_type i) { return _val[i]; }
  [[nodiscard]] constexpr const T& operator[](size_type i) const { return _val[i]; }

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


  // Iterators
  using iterator = T*;
  using const_iterator = const T*;

  [[nodiscard]] constexpr iterator begin() noexcept { return data(); }
  [[nodiscard]] constexpr const_iterator begin() const noexcept { return data(); }
  [[nodiscard]] constexpr const_iterator cbegin() const noexcept { return data(); }

  [[nodiscard]] constexpr iterator end() noexcept { return data() + size(); }
  [[nodiscard]] constexpr const_iterator end() const noexcept { return data() + size(); }
  [[nodiscard]] constexpr const_iterator cend() const noexcept { return data() + size(); }


  // Member Functions
  [[nodiscard]] static constexpr size_type size() noexcept { return 4; }
  [[nodiscard]] constexpr T* data() noexcept { return _val; }
  [[nodiscard]] constexpr const T* data() const noexcept { return _val; }

  constexpr void set(T vx, T vy, T vz, T vw) { x = vx; y = vy; z = vz; w = vw; }
  constexpr void set(const T* v) { set(v[0], v[1], v[2], v[3]); }
  constexpr void set(const T* v, T vw) { set(v[0], v[1], v[2], vw); }
  constexpr void set(const Vector3<T>& v, T vw) { set(v.x, v.y, v.z, vw); }
  constexpr void invert() { x = -x; y = -y; z = -z; w = -w; }

  // Vector2 swizzle
  [[nodiscard]] constexpr Vector2<T>& xy() {
    return *reinterpret_cast<Vector2<T>*>(data()); }
  [[nodiscard]] constexpr const Vector2<T>& xy() const {
    return *reinterpret_cast<const Vector2<T>*>(data()); }

  [[nodiscard]] constexpr Vector2<T>& yz() {
    return *reinterpret_cast<Vector2<T>*>(data()+1); }
  [[nodiscard]] constexpr const Vector2<T>& yz() const {
    return *reinterpret_cast<const Vector2<T>*>(data()+1); }

  [[nodiscard]] constexpr Vector2<T>& zw() {
    return *reinterpret_cast<Vector2<T>*>(data()+2); }
  [[nodiscard]] constexpr const Vector2<T>& zw() const {
    return *reinterpret_cast<const Vector2<T>*>(data()+2); }

  // Vector3 swizzle
  [[nodiscard]] constexpr Vector3<T>& xyz() {
    return *reinterpret_cast<Vector3<T>*>(data()); }
  [[nodiscard]] constexpr const Vector3<T>& xyz() const {
    return *reinterpret_cast<const Vector3<T>*>(data()); }

  [[nodiscard]] constexpr Vector3<T>& yzw() {
    return *reinterpret_cast<Vector3<T>*>(data()+1); }
  [[nodiscard]] constexpr const Vector3<T>& yzw() const {
    return *reinterpret_cast<const Vector3<T>*>(data()+1); }

  [[nodiscard]] constexpr Vector3<T>& rgb() {
    return *reinterpret_cast<Vector3<T>*>(data()); }
  [[nodiscard]] constexpr const Vector3<T>& rgb() const {
    return *reinterpret_cast<const Vector3<T>*>(data()); }
};


namespace gx {
  // **** Unary Operators ****
  template<typename T>
  [[nodiscard]] constexpr Vector2<T> operator-(const Vector2<T>& v) {
    return Vector2<T>(-v.x, -v.y); }

  template<typename T>
  [[nodiscard]] constexpr Vector3<T> operator-(const Vector3<T>& v) {
    return Vector3<T>(-v.x, -v.y, -v.z); }

  template<typename T>
  [[nodiscard]] constexpr Vector4<T> operator-(const Vector4<T>& v) {
    return Vector4<T>(-v.x, -v.y, -v.z, -v.w); }


  // **** Binary Operators ****
  template<typename T>
  [[nodiscard]] constexpr Vector2<T> operator+(const Vector2<T>& a, const Vector2<T>& b) {
    return Vector2<T>(a.x + b.x, a.y + b.y); }

  template<typename T>
  [[nodiscard]] constexpr Vector3<T> operator+(const Vector3<T>& a, const Vector3<T>& b) {
    return Vector3<T>(a.x + b.x, a.y + b.y, a.z + b.z); }

  template<typename T>
  [[nodiscard]] constexpr Vector4<T> operator+(const Vector4<T>& a, const Vector4<T>& b) {
    return Vector4<T>(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w); }


  template<typename T>
  [[nodiscard]] constexpr Vector2<T> operator-(const Vector2<T>& a, const Vector2<T>& b) {
    return Vector2<T>(a.x - b.x, a.y - b.y); }

  template<typename T>
  [[nodiscard]] constexpr Vector3<T> operator-(const Vector3<T>& a, const Vector3<T>& b) {
    return Vector3<T>(a.x - b.x, a.y - b.y, a.z - b.z); }

  template<typename T>
  [[nodiscard]] constexpr Vector4<T> operator-(const Vector4<T>& a, const Vector4<T>& b) {
    return Vector4<T>(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w); }


  template<typename T>
  [[nodiscard]] constexpr Vector2<T> operator*(const Vector2<T>& a, T b) {
    return Vector2<T>(a.x * b, a.y * b); }

  template<typename T>
  [[nodiscard]] constexpr Vector3<T> operator*(const Vector3<T>& a, T b) {
    return Vector3<T>(a.x * b, a.y * b, a.z * b); }

  template<typename T>
  [[nodiscard]] constexpr Vector4<T> operator*(const Vector4<T>& a, T b) {
    return Vector4<T>(a.x * b, a.y * b, a.z * b, a.w * b); }


  template<typename T>
  [[nodiscard]] constexpr Vector2<T> operator/(const Vector2<T>& a, T b) {
    return Vector2<T>(a.x / b, a.y / b); }

  template<typename T>
  [[nodiscard]] constexpr Vector3<T> operator/(const Vector3<T>& a, T b) {
    return Vector3<T>(a.x / b, a.y / b, a.z / b); }

  template<typename T>
  [[nodiscard]] constexpr Vector4<T> operator/(const Vector4<T>& a, T b) {
    return Vector4<T>(a.x / b, a.y / b, a.z / b, a.w / b); }


  // **** Template Stream Operators ****
  template<typename T>
  inline std::ostream& operator<<(std::ostream& out, const Vector2<T>& v) {
    return out << '[' << v.x << ' ' << v.y << ']'; }

  template<typename T>
  inline std::ostream& operator<<(std::ostream& out, const Vector3<T>& v) {
    return out << '[' << v.x << ' ' << v.y << ' ' << v.z << ']'; }

  template<typename T>
  inline std::ostream& operator<<(std::ostream& out, const Vector4<T>& v) {
    return out << '[' << v.x << ' ' << v.y << ' ' << v.z << ' ' << v.w << ']'; }


  // **** Template Functions ****
  template<typename T>
  [[nodiscard]] constexpr T PointDistanceSqr(const Vector2<T>& a, const Vector2<T>& b) {
    return Sqr(a.x - b.x) + Sqr(a.y - b.y); }

  template<typename T>
  [[nodiscard]] constexpr T PointDistanceSqr(const Vector3<T>& a, const Vector3<T>& b) {
    return Sqr(a.x - b.x) + Sqr(a.y - b.y) + Sqr(a.z - b.z); }

  template<typename T>
  [[nodiscard]] T PointDistance(const Vector2<T>& a, const Vector2<T>& b) {
    return std::sqrt(PointDistanceSqr(a, b)); }

  template<typename T>
  [[nodiscard]] T PointDistance(const Vector3<T>& a, const Vector3<T>& b) {
    return std::sqrt(PointDistanceSqr(a, b)); }

  template<typename T>
  [[nodiscard]] constexpr T DotProduct(const Vector2<T>& a, const Vector2<T>& b) {
    return (a.x * b.x) + (a.y * b.y); }

  template<typename T>
  [[nodiscard]] constexpr T DotProduct(const Vector3<T>& a, const Vector3<T>& b) {
    return (a.x * b.x) + (a.y * b.y) + (a.z * b.z); }

  template<typename T>
  [[nodiscard]] constexpr Vector3<T> CrossProduct(const Vector3<T>& a, const Vector3<T>& b)
  {
    return Vector3<T>((a.y * b.z) - (a.z * b.y),
                      (a.z * b.x) - (a.x * b.z),
                      (a.x * b.y) - (a.y * b.x));
  }

  template<typename T>
  [[nodiscard]] constexpr Vector2<T> UnitVec(const Vector2<T>& v) {
    return v * (static_cast<T>(1) / v.length()); }

  template<typename T>
  [[nodiscard]] constexpr Vector3<T> UnitVec(const Vector3<T>& v) {
    return v * (static_cast<T>(1) / v.length()); }
}
