//
// gx/DrawList.hh
// Copyright (C) 2021 Richard Bradley
//

#pragma once
#include "DrawEntry.hh"
#include <vector>
#include <map>


namespace gx {
  using DrawList = std::vector<DrawEntry>;
  using DrawListMap = std::map<int,DrawList>;
}
