//
// gx/Print.hh
// Copyright (C) 2026 Richard Bradley
//
// simplified stdout/stderr output
// (note that these functions are not equivalent to C++23 print/println)
//

#pragma once
#include <iostream>


namespace gx
{
  // value printing
  template<class T>
  void printval(std::ostream& os, T&& val) {
    using type = std::remove_reference_t<T>;
    if constexpr (std::is_enum_v<type>) {
      os << static_cast<std::underlying_type_t<type>>(val);
    } else {
      os << val;
    }
  }


  // print to ostream
  template<class... Args>
  void print_os(std::ostream& os, Args&&... args) {
    (printval(os, args),...);
  }

  template<class... Args>
  void println_os(std::ostream& os, Args&&... args) {
    (printval(os, args),...);
    os.put('\n');
  }


  // print to cout
  template<class... Args>
  void print(Args&&... args) {
    print_os(std::cout, args...);
  }

  template<class... Args>
  void println(Args&&... args) {
    println_os(std::cout, args...);
  }


  // print to cerr
  template<class... Args>
  void print_err(Args&&... args) {
    print_os(std::cerr, args...);
  }

  template<class... Args>
  void println_err(Args&&... args) {
    println_os(std::cerr, args...);
  }
}
