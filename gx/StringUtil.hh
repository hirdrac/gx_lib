//
// gx/StringUtil.hh
// Copyright (C) 2021 Richard Bradley
//

#pragma once
#include <sstream>


namespace gx
{
  template<typename... Args>
  [[nodiscard]] inline std::string concat(const Args&... args)
  {
    std::ostringstream os;
    ((os << args),...);
    return os.str();
  }
}
