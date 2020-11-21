//
// gx/print.hh
// Copyright (C) 2020 Richard Bradley
//
// simplified stdout/stderr output
//

#pragma once
#include <iostream>


namespace gx
{
  // print()
  template<typename... Args>
  inline void print(const Args&... args)
  {
    ((std::cout << args),...);
  }

  // println()
  template<typename... Args>
  inline void println(const Args&... args)
  {
    ((std::cout << args),...);
    std::cout.put('\n');
  }

  // print_err()
  template<typename... Args>
  inline void print_err(const Args&... args)
  {
    ((std::cerr << args),...);
  }

  // println_err()
  template<typename... Args>
  inline void println_err(const Args&... args)
  {
    ((std::cerr << args),...);
    std::cerr.put('\n');
  }

  // print_ostream()
  template<typename... Args>
  inline void print_ostream(std::ostream& os, const Args&... args)
  {
    ((os << args),...);
  }

  // println_ostream()
  template<typename... Args>
  inline void println_ostream(std::ostream& os, const Args&... args)
  {
    ((os << args),...);
    os.put('\n');
  }
}
