#ifndef PARALLELBRTREEBUILDER_H
#define PARALLELBRTREEBUILDER_H

#include <thrust/device_vector.h>


//BRTreeNode
struct BRTreeNode
{
  int left = -1;
  int right = -1;
};

class ParallelBRTreeBuilder
{
public:
  ParallelBRTreeBuilder(unsigned int* const sorted_morton_code, int size);
  void build();
  
  thrust::device_vector<BRTreeNode>* get_leaf_nodes();
  thrust::device_vector<BRTreeNode>* get_internal_nodes();
  
  ~ParallelBRTreeBuilder();
 

private:
  unsigned int* d_sorted_morton_code;
  thrust::device_vector<BRTreeNode> d_leaf_nodes;
  thrust::device_vector<BRTreeNode> h_leaf_nodes;
  thrust::device_vector<BRTreeNode> d_internal_nodes;
  thrust::device_vector<BRTreeNode> h_internal_nodes;
  
  int threadPerBlock;
  int numBlock;

  int numInternalNode;
  int numLeafNode;  
  
};

#endif
