#include "alias_table.h"

#include <cassert>
#include <cmath>
#include <vector>
#include <utility>

#include <iostream>
using std::cout;
using std::endl;

using std::vector;
using std::make_pair;
using std::pair;


namespace CMU462 { namespace StaticScene {

AliasTable::AliasTable(double *relativeWeights, int n) : n(n) {
  double sum = 0;
  for (int i = 0; i < n; i++) sum += relativeWeights[i];
  double invSum = 1.0 / sum;
  double avg = sum / n;

  vector<pair<double, int> > lightweights, heavyweights;
  for (int i = 0; i < n; i++) {
    const double p = relativeWeights[i];
    if (p > avg) {
      heavyweights.push_back(make_pair(p, i));
    } else {
      lightweights.push_back(make_pair(p, i));
    }
  }

  entries = new TableEntry[n];
  while (!lightweights.empty()) {
    const pair<double, int> pairL(lightweights.back());
    lightweights.pop_back();
    double pL = pairL.first;
    int iL = pairL.second;

    if (heavyweights.empty()) {
      entries[iL].firstPmf = relativeWeights[iL] * invSum;
      entries[iL].secondPmf = 0;
      entries[iL].ratio = 1;
      entries[iL].secondElem = -1;
    } else {
      const pair<double, int>& pairH = heavyweights.back();
      double pH = pairH.first;
      int iH = pairH.second;
      entries[iL].firstPmf = relativeWeights[iL] * invSum;
      entries[iL].secondPmf = relativeWeights[iH] * invSum;
      entries[iL].ratio = pL / avg;
      entries[iL].secondElem = iH;
      
      pH -= (avg - pL);
      if (pH <= avg) {
        heavyweights.pop_back();
        lightweights.push_back(make_pair(pH, iH));
      } else {
        heavyweights.back() = make_pair(pH, iH);
      }
    }
  }
  // Due to roundoff error, there might be one high val left.
  if (heavyweights.size() == 1) {
    int iH = heavyweights[0].second;
    entries[iH].firstPmf = relativeWeights[iH] * invSum;
    entries[iH].secondPmf = 0;
    entries[iH].ratio = 1;
    entries[iH].secondElem = -1;
  }

  delete relativeWeights;
}

AliasTable::~AliasTable() {
  delete entries;
}

int AliasTable::sample(float *pdf) {
  double d = random_uniform() * n;
  int i = (int) d;
  const TableEntry& entry = entries[i];
  if (d - i <= entry.ratio) {
    *pdf = entry.firstPmf;
    return i;
  } else {
    *pdf = entry.secondPmf;
    return entry.secondElem;
  }
}

} // namespace StaticScene
} // namespace CMU462
