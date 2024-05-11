//
// gx/StringUtil.hh
// Copyright (C) 2024 Richard Bradley
//

#pragma once
#include <string>
#include <string_view>
#include <sstream>
#include <limits>
#include <type_traits>
#include <cstdint>


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

  [[nodiscard]] constexpr std::string_view trimSpaces(std::string_view str)
  {
    std::size_t n = 0, len = str.length();
    while (len > 0 && str[n] == ' ') { ++n; --len; }
    while (len > 0 && str[len-1] == ' ') { --len; }
    return str.substr(n, len);
  }
}
