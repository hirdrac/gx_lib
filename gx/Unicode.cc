//
// gx/Unicode.cc
// Copyright (C) 2022 Richard Bradley
//

#include "Unicode.hh"


// ** NOTES **
// starting UTF-8 char:
//   0b0xxxxxxx  1 char encoding
//   0b110xxxxx  2 char encoding
//   0b1110xxxx  3 char encoding
//   0b11110xxx  4 char encoding
// additional char in multi-char encoding:
//   0b10xxxxxx

[[nodiscard]] static constexpr bool isMultiChar(int ch)
{
  return (ch & 0b11000000) == 0b10000000;
}


// functions
std::string gx::toUTF8(int32_t code)
{
  char str[8];
  char* ptr = str;
  if (code > 0) {
    if (code <= 0x7f) {
      *ptr++ = char(code);
    } else if (code <= 0x7ff) {
      *ptr++ = char(0b11000000 | (code>>6));
      *ptr++ = char(0b10000000 | (code&63));
    } else if (code <= 0xffff) {
      *ptr++ = char(0b11100000 | (code>>12));
      *ptr++ = char(0b10000000 | ((code>>6)&63));
      *ptr++ = char(0b10000000 | (code&63));
    } else if (code <= 0x10ffff) {
      *ptr++ = char(0b11110000 | (code>>18));
      *ptr++ = char(0b10000000 | ((code>>12)&63));
      *ptr++ = char(0b10000000 | ((code>>6)&63));
      *ptr++ = char(0b10000000 | (code&63));
    }
  } else if (code == 0) {
    // modified UTF-8 encoding of null character
    *ptr++ = char(0b11000000);
    *ptr++ = char(0b10000000);
  }
  return {str, ptr};
}

std::size_t gx::lengthUTF8(std::string_view sv)
{
  std::size_t len = 0;
  for (char ch : sv) { len += !isMultiChar(uint8_t(ch)); }
  return len;
}

std::size_t gx::indexUTF8(std::string_view sv, std::size_t pos)
{
  if (pos == std::string::npos) { return std::string::npos; }

  UTF8Iterator itr{sv};
  while (!itr.done() && pos > 0) { --pos; itr.next(); }
  return (pos != 0) ? std::string::npos : itr.pos();
}

void gx::popbackUTF8(std::string& str)
{
  while (!str.empty()) {
    const int ch = uint8_t(str.back());
    str.pop_back();

    // stop pop back if char isn't the 2/3/4 char in a multi-char encoding
    if (!isMultiChar(ch)) { break; }
  }
}

bool gx::eraseUTF8(std::string& str, std::size_t pos)
{
  if (pos == std::string::npos) { return false; }

  UTF8Iterator itr{str};
  while (pos > 0 && itr.next()) { --pos; }
  if (itr.done()) { return false; }

  const std::size_t i = itr.pos();
  itr.next();
  const std::size_t count = itr.done() ? std::string::npos : (itr.pos() - i);

  str.erase(i, count);
  return true;
}

bool gx::insertUTF8(std::string& str, std::size_t pos, int32_t code)
{
  const std::size_t i = indexUTF8(str, pos);
  if (i == std::string::npos) { return false; }
  str.insert(i, toUTF8(code));
  return true;
}


// UTF8Iterator implementation
bool gx::UTF8Iterator::next()
{
  if (_end) {
    if (_itr == _end) { return false; }
    do { ++_itr; } while ((_itr != _end) && isMultiChar(uint8_t(*_itr)));
  } else {
    if (*_itr == '\0') { return false; }
    do { ++_itr; } while (*_itr && isMultiChar(uint8_t(*_itr)));
  }
  return true;
}

int32_t gx::UTF8Iterator::get() const
{
  if (done()) { return -1; }

  const int ch = uint8_t(*_itr);
  if (ch < 0x80) {
    return ch;  // 1 byte ascii value
  }

  int bytes, minVal, val;
  if ((ch & 0b11100000) == 0b11000000) {
    bytes = 2;
    minVal = 0x80;
    val = ch & 0b11111;
  } else if ((ch & 0b11110000) == 0b11100000) {
    bytes = 3;
    minVal = 0x800;
    val = ch & 0b1111;
  } else if ((ch & 0b11111000) == 0b11110000) {
    bytes = 4;
    minVal = 0x10000;
    val = ch & 0b111;
  } else {
    return -1;
  }

  const char* ptr = _itr;
  const char* ptr_end = _itr + bytes;
  while (++ptr != ptr_end) {
    if (ptr == _end) { return -1; }
    const int c = uint8_t(*ptr);
    if (!isMultiChar(c)) { return -1; }
    val = (val << 6) | (c & 0b111111);
  }

  if (val < minVal && (bytes != 2 || val != 0)) {
    return -1; // reject overlong encoding except for 2 byte null encoding
  } else {
    return val;
  }
}
