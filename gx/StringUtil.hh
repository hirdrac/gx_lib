//
// gx/StringUtil.hh
// Copyright (C) 2025 Richard Bradley
//

#pragma once
#include <string>
#include <string_view>
#include <sstream>
#include <limits>
#include <type_traits>
#include <cstdint>
#include <cctype>


namespace gx
{
  template<class... Args>
  [[nodiscard]] std::string concat(const Args&... args)
  {
    if constexpr (sizeof...(args) == 0) {
      return {};
    } else {
      std::ostringstream os;
      ((os << args),...);
      return os.str();
    }
  }

  template<class IntType>
  [[nodiscard]] std::string formatInt(IntType n, char separator = ',')
  {
    static_assert(std::is_integral_v<IntType>);
    if (n == 0) { return {"0", 1ul}; }

    uint64_t v;
    static_assert(sizeof(IntType) <= sizeof(v));

    if constexpr (std::is_unsigned_v<IntType>) {
      v = n;
    } else if (n == std::numeric_limits<IntType>::min()) {
      v = uint64_t{std::numeric_limits<IntType>::max()} + 1;
    } else {
      v = static_cast<uint64_t>((n<0) ? -n : n);
    }

    char tmp[32];
    char* end = tmp + sizeof(tmp);
    char* ptr = end;

    int i = 0;
    for (; v > 0; v /= 10) {
      *(--ptr) = char('0' + (v % 10));
      if (v >= 10 && (++i % 3) == 0) { *(--ptr) = separator; }
    }

    if (n < 0) { *(--ptr) = '-'; }
    return {ptr, end};
  }

  [[nodiscard]] inline std::string formatHex(uint64_t val)
  {
    char buffer[16];
    char* end = buffer + std::size(buffer);
    char* ptr = end;
    do { *(--ptr) = "0123456789abcdef"[val&15]; val >>= 4; } while (val > 0);
    return {ptr, end};
  }

  [[nodiscard]] inline std::string formatHexUC(uint64_t val)
  {
    char buffer[16];
    char* end = buffer + std::size(buffer);
    char* ptr = end;
    do { *(--ptr) = "0123456789ABCDEF"[val&15]; val >>= 4; } while (val > 0);
    return {ptr, end};
  }

  [[nodiscard]] constexpr std::string_view trimSpaces(std::string_view str)
  {
    while (!str.empty() && str.front() == ' ') { str.remove_prefix(1); }
    while (!str.empty() && str.back() == ' ') { str.remove_suffix(1); }
    return str;
  }

  [[nodiscard]] constexpr uint64_t hashStr(std::string_view str)
  {
    // 64-bit FNV-1a hash function
    // https://en.wikipedia.org/wiki/Fowler-Noll-Vo_hash_function
    // (NOTE: not a cryptographic hash)

    const uint64_t offsetBasis = 14695981039346656037ull;
    const uint64_t prime = 1099511628211ull;

    uint64_t result = offsetBasis;
    for (char c : str) {
      result ^= uint8_t(c);
      result *= prime;
    }
    return result;
  }

  [[nodiscard]] constexpr uint64_t hashStrLC(std::string_view str)
  {
    // 64-bit FNV-1a hash function
    // https://en.wikipedia.org/wiki/Fowler-Noll-Vo_hash_function
    // (NOTE: not a cryptographic hash)

    const uint64_t offsetBasis = 14695981039346656037ull;
    const uint64_t prime = 1099511628211ull;

    uint64_t result = offsetBasis;
    for (char c : str) {
      if (c >= 'A' && c <= 'Z') { c += 32; } // tolower
      result ^= uint8_t(c);
      result *= prime;
    }
    return result;
  }

  [[nodiscard]] inline std::string toUpper(std::string_view sv)
  {
    // only supports ASCII
    std::string s{sv};
    for (char& c : s) { c = char(std::toupper(static_cast<unsigned char>(c))); }
    return s;
  }

  [[nodiscard]] inline std::string toLower(std::string_view sv)
  {
    // only supports ASCII
    std::string s{sv};
    for (char& c : s) { c = char(std::tolower(static_cast<unsigned char>(c))); }
    return s;
  }
}
