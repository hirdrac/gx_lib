//
// gx/DrawList.hh
// Copyright (C) 2024 Richard Bradley
//

#pragma once
#include "DrawEntry.hh"
#include "Types.hh"
#include <initializer_list>
#include <vector>

namespace gx {
  class DrawList;
}

class gx::DrawList
{
 public:
  [[nodiscard]] const DrawEntry* data() const { return _data.data(); }
  [[nodiscard]] std::size_t size() const { return _data.size(); }
  [[nodiscard]] bool empty() const { return _data.empty(); }

  void clear() { _data.clear(); }

  void append(const DrawList& dl) {
    const DrawEntry* d = dl.data();
    _data.insert(_data.end(), d, d + dl.size());
  }

  void add(DrawCmd cmd, const Mat4& m1, const Mat4& m2) {
    _data.reserve(_data.size() + 33);
    _data.push_back(cmd);
    _data.insert(_data.end(), m1.begin(), m1.end());
    _data.insert(_data.end(), m2.begin(), m2.end());
  }

  template<class... Args>
  void add(DrawCmd cmd, const Args&... args) {
    if constexpr (sizeof...(args) == 0) {
      _data.push_back(cmd);
    } else {
      const std::initializer_list<DrawEntry> x {cmd, args...};
      _data.insert(_data.end(), x.begin(), x.end());
    }
  }

 private:
  std::vector<DrawEntry> _data;
};
