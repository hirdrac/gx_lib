//
// gx/IDRegion.hh
// Copyright (C) 2026 Richard Bradley
//
// Track screen coords of selectable text
// (text marked with <id=X></id> meta tags)
//

#pragma once
#include "Rect.hh"
#include "Types.hh"
#include <vector>

namespace gx {
  struct IDRegion {
    int64_t id;
    Rect region;
  };
}

class gx::IDRegionList
{
 public:
  using storage_type = std::vector<IDRegion>;

  // vector-like types & functions
  using const_iterator = storage_type::const_iterator;
  using size_type = storage_type::size_type;

  [[nodiscard]] const_iterator begin() const { return _data.begin(); }
  [[nodiscard]] const_iterator end() const { return _data.end(); }

  [[nodiscard]] const IDRegion* data() const { return _data.data(); }
  [[nodiscard]] size_type size() const { return _data.size(); }

  [[nodiscard]] bool empty() const { return _data.empty(); }
  void clear() { _data.clear(); }

  [[nodiscard]] size_type capacity() const { return _data.capacity(); }
  void reserve(size_type cap) { _data.reserve(cap); }

  // methods
  void add(int64_t id, const Rect& region) {
    _data.push_back({id, region});
  }

  void add(int64_t id, Vec2 a, Vec2 b) {
    _data.push_back(
      {id, {std::min(a.x, b.x), std::min(a.y, b.y),
         abs(a.x - b.x), abs(a.y - b.y)}});
  }

  [[nodiscard]] int64_t selected(Vec2 pt) const {
    for (auto& e : _data) {
      if (e.region.contains(pt)) { return e.id; }
    }
    return 0;
  }

  [[nodiscard]] Rect getRegion(int64_t id) const {
    for (const IDRegion& r : _data) {
      if (r.id == id) { return r.region; }
    }
    return {};
  }

 private:
  storage_type _data;
};
