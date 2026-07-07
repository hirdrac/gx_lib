//
// gx/Random.hh
// Copyright (C) 2026 Richard Bradley
//
// Pseudo-random number generator functions/type
//

#pragma once
#include <random>
#include <concepts>
#include <cstdint>


namespace gx {
  // Functions
  uint32_t genRandomSeed32();
  uint64_t genRandomSeed64();

  class RandomSequence;
}


// Types
class gx::RandomSequence
{
 public:
  using engine_type = std::mt19937_64;
  using seed_type = uint64_t;

  RandomSequence() : _seed{genRandomSeed64()}, _engine{_seed} { }
  RandomSequence(seed_type seed_val) : _seed{seed_val}, _engine{_seed} { }

  [[nodiscard]] seed_type seed() const { return _seed; }
  [[nodiscard]] engine_type& engine() { return _engine; }

  void reset() { _engine.seed(_seed); }

  seed_type generate() { return _engine(); }

  template<class dist_type>
  auto generate(dist_type&& d) { return d(_engine); }

  template<std::floating_point T>
  [[nodiscard]] T generate(T min, T max) {
    return std::uniform_real_distribution{min,max}(_engine);
  }

  template<std::integral T>
  [[nodiscard]] T generate(T min, T max) {
    return std::uniform_int_distribution{min,max}(_engine);
  }

  void skip(std::size_t num = 1) { _engine.discard(num); }

 private:
  seed_type _seed;
  engine_type _engine;
};
