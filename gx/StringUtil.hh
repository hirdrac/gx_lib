//
// gx/StringUtil.hh
// Copyright (C) 2021 Richard Bradley
//

#pragma once
#include <string>
#include <sstream>
#include <limits>
#include <type_traits>
#include <cstdint>


namespace gx
{
  template<typename... Args>
  [[nodiscard]] inline std::string concat(const Args&... args)
  {
    std::ostringstream os;
    ((os << args),...);
    return os.str();
  }

  template<typename IntType>
  [[nodiscard]] std::string formatInt(IntType n, char separator = ',')
  {
    static_assert(std::is_integral_v<IntType>);
    if (n == 0) { return "0"; }

    uint64_t v;
    static_assert(sizeof(IntType) <= sizeof(v));

    if constexpr (std::is_unsigned_v<IntType>) {
      v = n;
    } else if (n == std::numeric_limits<IntType>::min()) {
      v = uint64_t{std::numeric_limits<IntType>::max()} + 1;
    } else {
      v = (n<0) ? -n : n;
    }

    char tmp[32];
    char* ptr = &tmp[sizeof(tmp) - 1];
    *ptr = '\0';

    int i = 0;
    for (; v > 0; v /= 10) {
      *(--ptr) = '0' + (v % 10);
      if (v >= 10 && (++i % 3) == 0) { *(--ptr) = separator; }
    }

    if (n < 0) { *(--ptr) = '-'; }
    return ptr;
  }
}
