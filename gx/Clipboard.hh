//
// gx/Clipboard.hh
// Copyright (C) 2025 Richard Bradley
//
// Clipboard handling functions
//

#pragma once
#include <string>


namespace gx {
  std::string getClipboardFull();
  std::string getClipboardFirstLine();

  void setClipboard(const char* s);

  template<class T>
  void setClipboard(const T& s) { setClipboard(s.c_str()); }
}
