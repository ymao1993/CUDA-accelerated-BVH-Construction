#ifndef CMU462_RANDOMUTIL_H
#define CMU462_RANDOMUTIL_H

#include <cstdlib>

namespace CMU462 {

/**
 * Returns a number distributed uniformly over [0, 1].
 */
inline double random_uniform() {
  return ((double)std::rand()) / RAND_MAX;
}

/**
 * Returns true with probability p and false with probability 1 - p.
 */
inline bool coin_flip(double p) {
  return random_uniform() < p;
}

} // namespace CMU462

#endif  // CMU462_RANDOMUTIL_H
