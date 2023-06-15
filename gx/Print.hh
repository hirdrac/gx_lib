//
// gx/Print.hh
// Copyright (C) 2023 Richard Bradley
//
// simplified stdout/stderr output
// (note that these functions are not equivalent to C++23 print/println)
//

#pragma once
#include <iostream>


namespace gx
{
  // print()
  template<typename... Args>
  inline void print(std::ostream& os, const Args&... args)
  {
    ((os << args),...);
  }

  template<typename... Args>
  inline void print(const Args&... args)
  {
    print(std::cout, args...);
  }

  // println()
  template<typename... Args>
  inline void println(std::ostream& os, const Args&... args)
  {
    ((os << args),...);
    os.put('\n');
  }

  template<typename... Args>
  inline void println(const Args&... args)
  {
    println(std::cout, args...);
  }


  // std error output
  template<typename... Args>
  inline void print_err(const Args&... args)
  {
    print(std::cerr, args...);
  }

  template<typename... Args>
  inline void println_err(const Args&... args)
  {
    println(std::cerr, args...);
  }
}
