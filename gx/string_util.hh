//
// gx/string_util.hh
// Copyright (C) 2020 Richard Bradley
//

#pragma once
#include <sstream>


namespace gx
{
  template<typename... Args>
  inline std::string concat(const Args&... args)
  {
    std::ostringstream os;
    ((os << args),...);
    return os.str();
  }
}
