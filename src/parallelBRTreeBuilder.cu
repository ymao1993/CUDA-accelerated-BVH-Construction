#include "parallelBRTreeBuilder.h"
#include <thrust/sequence.h>
#include <iostream>

#define DEFAULT_THREAD_PER_BLOCK 256

/**
 * intialize parallelBRTreeBuilder by copying the data needed
 * from host memory (CPU) to device memory (GPU), initialize
 * data members such as configuration parameters.
 */
ParallelBRTreeBuilder::ParallelBRTreeBuilder(unsigned int* const sorted_morton_code, int size):
numInternalNode(size-1),
numLeafNode(size)
{
   //copy data from cpu to gpu
   cudaMalloc(&d_sorted_morton_code, size * sizeof(unsigned int));
   cudaMemcpy(d_sorted_morton_code, sorted_morton_code, size * sizeof(unsigned int), cudaMemcpyHostToDevice);
   
   //resize leaf arrays and internal node arrays
   d_leaf_nodes.resize(numLeafNode);
   d_internal_nodes.resize(numInternalNode);
   
   //set configuration parameter for CUDA
   threadPerBlock = DEFAULT_THREAD_PER_BLOCK;
   numBlock = (numInternalNode+DEFAULT_THREAD_PER_BLOCK-1)/threadPerBlock; 
   
   std::cout<<std::endl;
   std::cout<<"-threads per block:"<<threadPerBlock<<std::endl;
   std::cout<<"-number of blocks:"<<numBlock<<std::endl;
   
}


//FOR BR-TREE CONSTRUCTION
//TODO: implement internal node processing routine
//TODO: handle duplicated morton codes as special case (using their position i,j as fallback)

//FOR BVH CONSTRUCTION
//TODO: implement AABB construction process by go back from the tree node to the root
//TODO: convert BR-TREE BACK TO BVH
//TODO: debug
__global__ static void processInternalNode(unsigned int* sorted_morton_code, int numInternalNode)
{
  int index = blockIdx.x * blockDim.x + threadIdx.x;
  if(index >= numInternalNode ) return;
  
  
}

/**
 * build binary radix tree on GPU
 */
void ParallelBRTreeBuilder::build()
{
  processInternalNode<<<numBlock,threadPerBlock>>>(d_sorted_morton_code, numInternalNode);
}

/**
 * get the leaf nodes (host)
 */
thrust::device_vector<BRTreeNode>* ParallelBRTreeBuilder::get_leaf_nodes()
{
  h_leaf_nodes.resize(numLeafNode);
  thrust::copy(d_leaf_nodes.begin(), d_leaf_nodes.end(), h_leaf_nodes.begin());   
  return &h_leaf_nodes;
}

/**
 * get the internal nodes (host)
 */
thrust::device_vector<BRTreeNode>* ParallelBRTreeBuilder::get_internal_nodes()
{
  h_internal_nodes.resize(numInternalNode);
  thrust::copy(d_internal_nodes.begin(), d_internal_nodes.end(), h_internal_nodes.begin());   
  return &h_internal_nodes;
}

/**
 * deconstructor
 */
ParallelBRTreeBuilder::~ParallelBRTreeBuilder()
{
  //TODO: free stuffs here
}


