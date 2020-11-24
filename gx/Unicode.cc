//
// gx/Unicode.cc
// Copyright (C) 2020 Richard Bradley
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


// functions
std::string gx::toUTF8(int num)
{
  char str[8];
  char* ptr = str;
  if (num > 0) {
    if (num <= 0x7f) {
      *ptr++ = char(num);
    } else if (num <= 0x7ff) {
      *ptr++ = char(0b11000000 | (num>>6));
      *ptr++ = char(0b10000000 | (num&63));
    } else if (num <= 0xffff) {
      *ptr++ = char(0b11100000 | (num>>12));
      *ptr++ = char(0b10000000 | ((num>>6)&63));
      *ptr++ = char(0b10000000 | (num&63));
    } else if (num <= 0x10ffff) {
      *ptr++ = char(0b11110000 | (num>>18));
      *ptr++ = char(0b10000000 | ((num>>12)&63));
      *ptr++ = char(0b10000000 | ((num>>6)&63));
      *ptr++ = char(0b10000000 | (num&63));
    }
  } else if (num == 0) {
    // modified UTF-8 encoding of null character
    *ptr++ = char(0b11000000);
    *ptr++ = char(0b10000000);
  }
  return std::string(str, ptr);
}

int gx::lengthUTF8(std::string_view sv)
{
  int len = 0;
  for (UTF8Iterator itr(sv); !itr.done(); itr.next()) { ++len; }
  return len;
}

void gx::popbackUTF8(std::string& str)
{
  while (!str.empty()) {
    int c = str.back();
    str.pop_back();

    // stop pop back if char isn't the 2/3/4 char in a multi-char encoding
    if ((c & 0b11000000) != 0b10000000) { break; }
  }
}


// UTF8Iterator implementation
bool gx::UTF8Iterator::next()
{
  if (done()) { return false; }
  do {
    ++_itr;
    if (done()) { return false; }
  } while ((uint8_t(*_itr) & 0b11000000) == 0b10000000);
  return true;
}

int gx::UTF8Iterator::get() const
{
  if (done()) { return -1; }

  int ch = uint8_t(*_itr);
  if (ch < 0x80) {
    return ch;
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
    ch = uint8_t(*ptr);
    if ((ch & 0b11000000) != 0b10000000) { return -1; }
    val = (val << 6) | (ch & 0b111111);
  }

  if (val < minVal && (bytes != 2 || val != 0)) {
    return -1; // reject overlong encoding except for 2 byte null encoding
  } else {
    return val;
  }
}