//
// gx/Unicode.hh
// Copyright (C) 2025 Richard Bradley
//
// Unicode/UTF-8 utilities
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

  [[nodiscard]] std::size_t lengthUTF8(std::string_view sv);
    // returns UTF-8 string length

  [[nodiscard]] std::size_t indexUTF8(std::string_view sv, std::size_t pos);
    // returns string index of specified position for UTF-8 encoded string
    // (returns string::npos if pos >= number of UTF-8 encoded chars in input)

  bool eraseUTF8(std::string& str, std::size_t pos, std::size_t len);
    // removes len character(s) starting at pos
    // (len == string::npos will delete all characters from pos to end)

  bool insertUTF8(std::string& str, std::size_t pos, int32_t code);
    // adds UTF-8 character to UTF-8 string at specified character position

  [[nodiscard]] std::string_view substrUTF8(
    std::string_view sv, std::size_t pos, std::size_t len);
    // returns sub-view of UTF-8 encoded string
    // (returns empty string_view if pos > utf8length)

  [[nodiscard]] std::size_t findUTF8(
    std::string_view sv, int32_t code, std::size_t start = 0);
    // returns index of first occurence of character in UTF-8 encoded string
    // (search begins at 'start' byte position)

  class UTF8Iterator;
}


// types
class gx::UTF8Iterator
{
 public:
  // constructors
  UTF8Iterator(const char* s) noexcept
    : _begin{s}, _itr{s}, _end{nullptr} { }
  UTF8Iterator(const char* s, const char* e) noexcept
    : _begin{s}, _itr{s}, _end{e} { }
  UTF8Iterator(const char* data, std::size_t size) noexcept
    : _begin{data}, _itr{data}, _end{data + size} { }

  template<class T>
  explicit UTF8Iterator(const T& s)
    : UTF8Iterator{std::data(s), std::size(s)} { }


  // member functions
  [[nodiscard]] bool done() const {
    return (_itr == _end) || (!_end && *_itr == '\0'); }
    // return true if there is no more data to read

  bool next();
    // advance to next character, returns false if at the end

  [[nodiscard]] int32_t get() const;
    // returns current unicode character
    // (returns -1 for an invalid encoding, 0 for end of string)

  [[nodiscard]] std::size_t pos() const { return std::size_t(_itr - _begin); }
    // returns byte position of iterator
    // (useful for substrings, but don't use for counting unicode characters)

  void setPos(std::size_t p) { _itr = _begin + p; }
    // reset internal byte position

  // helper operators
  [[nodiscard]] explicit operator bool() const { return !done(); }
  UTF8Iterator& operator++() { next(); return *this; }
  [[nodiscard]] int32_t operator*() const { return get(); }

 private:
  const char* _begin;
  const char* _itr;
  const char* _end;
};
