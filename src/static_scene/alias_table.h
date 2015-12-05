#ifndef CMU462_STATICSCENE_ALIASTABLE_H
#define CMU462_STATICSCENE_ALIASTABLE_H

#include "../random_util.h"

namespace CMU462 { namespace StaticScene {

/**
 * Represents a discrete probability distribution in a way that can be
 * efficiently sampled.
 */
class AliasTable {
 public:
  /**
   * relativeWeights is an un-normalized pmf for the distribution, and n is the
   * number of items. This takes ownership over relativeWeights, so the caller
   * doesn't need to delete it.
   */
  AliasTable(double *relativeWeights, int n);
  ~AliasTable();

  /**
   * Returns a number from 0 to n-1 and writes its probability into the output
   * pointer.
   */
  int sample(float *pdf);

 private:
  struct TableEntry {
    float firstPmf;
    float secondPmf;
    float ratio; // ratio of probabilities for just this column.
    int secondElem;
  };

  TableEntry *entries;
  int n;
};

} // namespace StaticScene
} // namespace CMU462

#endif // CMU462_STATICSCENE_ALIASTABLE_H
