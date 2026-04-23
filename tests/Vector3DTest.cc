//
// Vector3DTest.cc
// Copyright (C) 2026 Richard Bradley
//

#include "gx/Vector3D.hh"
#include <cassert>
using namespace gx;

#ifdef NDEBUG
#error "can't run test with NDEBUG"
#endif


using Vec2 = Vector2<float>;
using Vec3 = Vector3<float>;
using Vec4 = Vector4<float>;


template<class T>
void test_compare(const T& v2)
{
  const T v0, v1;

  assert(v0 == v0);
  assert(!(v0 != v0));
  assert(v0 == v1);
  assert(!(v0 != v1));

  assert(v2 == v2);
  assert(!(v2 != v2));
  assert(v0 != v2);
  assert(!(v0 == v2));
}


int main(int argc, char** argv)
{
  // == and != operators
  test_compare<Vec2>({1,2});
  test_compare<Vec3>({1,2,3});
  test_compare<Vec4>({1,2,3,4});

  return 0;
}
