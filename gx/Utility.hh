//
// gx/Utility.hh
// Copyright (C) 2022 Richard Bradley
//

#pragma once
#include <type_traits>


namespace gx {
  // equivalent to C++23 std::exchange
  // (replace with that version when available)
  //
  template<class T, class U = T>
  constexpr T exchange(T& obj, U&& new_value)
    noexcept(std::is_nothrow_move_constructible<T>::value
             && std::is_nothrow_assignable<T&, U>::value)
  {
    const T old_value = std::move(obj);
    obj = std::forward<U>(new_value);
    return old_value;
  }
}
