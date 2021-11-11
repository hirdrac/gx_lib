//
// gx/Unicode.hh
// Copyright (C) 2021 Richard Bradley
//
// Unicode/UTF-8 utilities
// (C++17 version)
//

#pragma once
#include <string>
#include <string_view>
#include <cstdint>


namespace gx
{
  // functions
  [[nodiscard]] std::string toUTF8(int32_t code);
  [[nodiscard]] inline std::string toUTF8(uint32_t code) {
    return toUTF8(int32_t(code)); }
    // returns UTF-8 encoded value (1-4 characters) of a unicode character

  [[nodiscard]] int lengthUTF8(std::string_view str);
    // returns UTF8 string length

  void popbackUTF8(std::string& str);
    // removes last trailing UTF-8 character

  class UTF8Iterator;
}


// types
class gx::UTF8Iterator
{
 public:
  // constructors
  UTF8Iterator(const char* s) noexcept
    : _itr{s}, _end{nullptr} { }
  UTF8Iterator(const char* s, const char* e) noexcept
    : _itr{s}, _end{e} { }

  template<class T>
  explicit UTF8Iterator(const T& s)
    : _itr{std::data(s)}, _end{_itr + std::size(s)} { }


  // member functions
  [[nodiscard]] bool done() const {
    return (_itr == _end) || (!_end && *_itr == '\0'); }
    // return true if there is no more data to read

  bool next();
    // advance to next character, return false if already at the end

  [[nodiscard]] int32_t get() const;
    // return current unicode character
    // (returns -1 for an invalid encoding or at the end of the string)

 private:
  const char* _itr;
  const char* _end;
};
