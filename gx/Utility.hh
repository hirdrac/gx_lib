//
// gx/Utility.hh
// Copyright (C) 2022 Richard Bradley
//

#pragma once
#include <type_traits>


namespace gx {
  // C++23 equivalent functions

  template<class T, class U = T>
  constexpr T exchange(T& obj, U&& new_value)
    noexcept(std::is_nothrow_move_constructible<T>::value
             && std::is_nothrow_assignable<T&, U>::value)
  {
    const T old_value = std::move(obj);
    obj = std::forward<U>(new_value);
    return old_value;
  }

  [[noreturn]] inline void unreachable()
  {
#ifdef __GNUC__ // GCC, Clang, ICC
    __builtin_unreachable();
#elif defined(_MSC_VER) // MSVC
    __assume(false);
#endif
  }
}
