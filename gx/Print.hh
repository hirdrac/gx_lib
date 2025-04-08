//
// gx/Print.hh
// Copyright (C) 2025 Richard Bradley
//
// simplified stdout/stderr output
// (note that these functions are not equivalent to C++23 print/println)
//

#pragma once
#include <iostream>


namespace gx
{
  // print()
  template<class... Args>
  void print(std::ostream& os, Args&&... args)
  {
    (os << ... << args);
  }

  template<class... Args>
  void print(Args&&... args)
  {
    print(std::cout, args...);
  }

  // println()
  template<class... Args>
  void println(std::ostream& os, Args&&... args)
  {
    (os << ... << args) << '\n';
  }

  template<class... Args>
  void println(Args&&... args)
  {
    println(std::cout, args...);
  }


  // std error output
  template<class... Args>
  void print_err(Args&&... args)
  {
    print(std::cerr, args...);
  }

  template<class... Args>
  void println_err(Args&&... args)
  {
    println(std::cerr, args...);
  }
}
