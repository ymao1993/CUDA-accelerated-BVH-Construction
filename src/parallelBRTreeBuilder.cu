#include "parallelBRTreeBuilder.h"

/**
 * intialize parallelBRTreeBuilder by copying the data needed
 * from host memory (CPU) to device memory (GPU).
 */
ParallelBRTreeBuilder::ParallelBRTreeBuilder(unsigned int* sorted_morton_code, int size)
{ 
   d_sorted_morton_code.resize(size);
   thrust::copy(sorted_morton_code, sorted_morton_code + size, d_sorted_morton_code.begin());
   
}

