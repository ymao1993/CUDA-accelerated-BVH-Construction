#ifndef PARALLELBRTREEBUILDER_H
#define PARALLELBRTREEBUILDER_H

#include <thrust/device_vector.h>
#include <thrust/transform.h>

class ParallelBRTreeBuilder
{
public:
  ParallelBRTreeBuilder(unsigned int* sorted_morton_code, int size);

private:
  thrust::device_vector<int> d_sorted_morton_code;
  
};

#endif
