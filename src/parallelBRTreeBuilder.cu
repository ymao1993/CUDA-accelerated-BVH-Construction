#include "parallelBRTreeBuilder.h"
#include <iostream>

#define DEFAULT_THREAD_PER_BLOCK 1024

/*check error code of cudaMalloc and print out if needed*/
#define safe_cuda(CODE)\
 {\
  cudaError_t err = CODE;\
  if(err != cudaSuccess) {\
    std::cout<<"CUDA error:"<<cudaGetErrorString(err)<<std::endl;\
 }\
}

/**
 * alloc a memory on gpu and copy data from cpu to gpu.
 */
inline void copyFromCPUtoGPU(void** dst, void* src, int size)
{
   cudaMalloc(dst, size);
   safe_cuda(cudaMemcpy(*dst, src, size, cudaMemcpyHostToDevice));
}

/**
 * alloc a memory on cpu and copy data from gpu to cpu.
 */
inline void copyFromGPUtoCPU(void** dst, void* src, int size)
{
   *dst = malloc(size);
   safe_cuda(cudaMemcpy(*dst, src, size, cudaMemcpyDeviceToHost));
}

/**
 * intialize parallelBRTreeBuilder by copying the data needed
 * from host memory (CPU) to device memory (GPU), initialize
 * data members such as configuration parameters.
 */
ParallelBRTreeBuilder::ParallelBRTreeBuilder(unsigned int* const sorted_morton_code, BBox* const bboxes, int size):
 d_sorted_morton_code(0),
 d_leaf_nodes(0),
 h_leaf_nodes(0),
 d_internal_nodes(0),
 h_internal_nodes(0), 
 numInternalNode(size-1),
 numLeafNode(size)
{
   //copy data from cpu to gpu
   copyFromCPUtoGPU((void**)&d_sorted_morton_code, sorted_morton_code, size * sizeof(unsigned int));
   copyFromCPUtoGPU((void**)&d_bboxes, bboxes, size * sizeof(BBox));
   
   //initialize d_leaf_nodes and d_internal_nodes
   h_leaf_nodes = (BRTreeNode*)calloc(numLeafNode, sizeof(BRTreeNode));
   for (int idx = 0; idx < numLeafNode; idx++) { 
      h_leaf_nodes[idx].setIdx(idx);
      h_leaf_nodes[idx].bbox = BBox();
   }
   copyFromCPUtoGPU((void**)&d_leaf_nodes,h_leaf_nodes,numLeafNode * sizeof(BRTreeNode));
   free(h_leaf_nodes);

   h_internal_nodes = (BRTreeNode*)calloc(numInternalNode, sizeof(BRTreeNode));
   for (int idx = 0; idx < numInternalNode; idx++) { 
      h_internal_nodes[idx].setIdx(idx);
      h_internal_nodes[idx].bbox = BBox();
   }
   copyFromCPUtoGPU((void**)&d_internal_nodes,h_internal_nodes,numInternalNode * sizeof(BRTreeNode));
   free(h_internal_nodes);
}

/**
 * delta operator measures the common prefix of two morton_code
 * if j is not in the range of the sorted_morton_code,
 * delta operator returns -1.
 */
__device__ int delta(int i, int j, unsigned int* sorted_morton_code, int length)
{
  if(j<0||j>=length)
  {
    return -1;
  }
  else
  {
    return __clz(sorted_morton_code[i] ^ sorted_morton_code[j]);
  }
}

/**
 * determine the range of an internal node
 */
__device__ int2 determineRange(unsigned int* sorted_morton_code, int numInternalNode, int i)
{
  int size = numInternalNode+1;
  int d = delta(i, i+1, sorted_morton_code, size) - delta(i, i-1, sorted_morton_code, size);
  d = d > 0? 1:-1;
  
  //compute the upper bound for the length of the range
  int delta_min = delta(i,i-d,sorted_morton_code, size);
  int lmax = 2;
  while(delta(i,i+lmax*d,sorted_morton_code,size)>delta_min)
  {
    lmax = lmax * 2;
  }
  
  //find the other end using binary search
  int l=0;
  for(int t = lmax/2; t>=1 ;t/=2)
  {
    if(delta(i,i+(l+t)*d,sorted_morton_code,size)>delta_min)
    {
      l= l+t;
    }
  }
  int j = i+l*d;
  
  int2 range;
  if(i<=j) { range.x = i; range.y = j; }
  else     { range.x = j; range.y = i; }
  return range;
}

/**
 * to judge if two values differes 
 * in bit position n
 */
__device__ bool is_diff_at_bit(unsigned int val1, unsigned int val2, int n)
{
  return val1>>(31-n) != val2>>(31-n);
}

/**
 * find the best split position for an internal node
 */
__device__ int findSplit(unsigned int* sorted_morton_code, int start, int last)
{
  //return -1 if there is only 
  //one primitive under this node.
  if(start == last) 
  {
    return -1;
  }
  else
  {
    int common_prefix = __clz(sorted_morton_code[start] ^ sorted_morton_code[last]);

    //handle duplicated morton code separately
    if(common_prefix == 32)
    {
      return (start + last)/2;
    }

    // Use binary search to find where the next bit differs.
    // Specifically, we are looking for the highest object that
    // shares more than commonPrefix bits with the first one.

    int split = start; // initial guess
    int step = last - start;
    do
    {
        step = (step + 1) >> 1; // exponential decrease
        int newSplit = split + step; // proposed new position

        if (newSplit < last)
        {
            bool is_diff = is_diff_at_bit(sorted_morton_code[start],
                                          sorted_morton_code[newSplit],
                                          common_prefix);
            if(!is_diff)
            {
              split = newSplit; // accept proposal
            }
        }
    }
    while (step > 1);

    return split;
  }
}

//FOR BR-TREE CONSTRUCTION
//TODO: implement internal node processing routine
//TODO: handle duplicated morton codes as special case (using their position i,j as fallback)

//FOR BVH CONSTRUCTION
//TODO: implement AABB construction process by go back from the tree node to the root
//TODO: convert BR-TREE BACK TO BVH
//TODO: debug
__global__ static void processInternalNode(unsigned int* sorted_morton_code, int numInternalNode,
					   BRTreeNode* leafNodes,
				           BRTreeNode* internalNodes)
{
  int idx = blockIdx.x * blockDim.x + threadIdx.x;
  if(idx >= numInternalNode ) return;
  
  // Find out which range of objects the node corresponds to.
  int2 range = determineRange(sorted_morton_code, numInternalNode, idx);
  int first = range.x;
  int last = range.y;

  // Determine where to split the range.
  int split = findSplit(sorted_morton_code, first, last);

  if(split == -1) return;

  // Select childA.
  BRTreeNode* childA;
  bool isChildALeaf = false;
  if (split == first) {
      childA = &(leafNodes[split]);
      isChildALeaf = true;
  } else childA = &(internalNodes[split]);

  // Select childB.
  BRTreeNode* childB;
  bool isChildBLeaf = false;
  if (split + 1 == last) {
      childB = &(leafNodes[split + 1]);
      isChildBLeaf = true;
  }
  else childB = &(internalNodes[split + 1]);

  // Record parent-child relationships.
  internalNodes[idx].setChildA(split,isChildALeaf);
  internalNodes[idx].setChildB(split+1,isChildBLeaf);
  childA->setParent(idx);
  childB->setParent(idx);
}

/**
 * construct bounding boxes from leaf up to root
 */
__global__ static void calculateBoudingBox(BBox* d_bboxes, int numLeafNode,
			                   BRTreeNode* leafNodes, BRTreeNode* internalNodes)
{
  int idx = blockIdx.x * blockDim.x + threadIdx.x;
  if(idx >= numLeafNode ) return;
  
  //handle leaf first
  BRTreeNode* node = &leafNodes[idx];
  node->bbox = d_bboxes[idx];

  //terminate if it is root node
  bool is_null = false;
  int parentIdx = node->getParent(is_null);
  if(is_null) return; 
  node = &internalNodes[parentIdx];

  int initial_val = atomicInc(&node->counter,1);  
  while(1)
  {
    if(initial_val == 0) return; //terminate the first accesing thread
    
    //calculate bounding box by merging two children's bounding box
    bool is_leaf = false;  
    int childAIdx = node->getChildA(is_leaf, is_null);
    if(is_leaf) node->bbox.expand(leafNodes[childAIdx].bbox);
    else node->bbox.expand(internalNodes[childAIdx].bbox);

    int childBIdx = node->getChildB(is_leaf, is_null);
    if(is_leaf) node->bbox.expand(leafNodes[childBIdx].bbox);
    else node->bbox.expand(internalNodes[childBIdx].bbox); 
    
    //terminate if it is root node
    parentIdx = node->getParent(is_null);
    if(is_null) return; 
    node = &internalNodes[parentIdx];   
    initial_val = atomicInc(&node->counter,1);
  } 
}

/**
 * build binary radix tree on GPU
 */
void ParallelBRTreeBuilder::build()
{
  //build the bvh
  int threadPerBlock = DEFAULT_THREAD_PER_BLOCK;
  int numBlock = (numInternalNode+DEFAULT_THREAD_PER_BLOCK-1)/threadPerBlock; 
  processInternalNode<<<numBlock,threadPerBlock>>>(d_sorted_morton_code, numInternalNode,
						   d_leaf_nodes, d_internal_nodes);
  
  //calculate bounding box
  threadPerBlock = DEFAULT_THREAD_PER_BLOCK;
  numBlock = (numLeafNode+DEFAULT_THREAD_PER_BLOCK-1)/threadPerBlock; 
  calculateBoudingBox<<<numBlock,threadPerBlock>>>(d_bboxes, numLeafNode, 
						   d_leaf_nodes, d_internal_nodes);
}

/**
 * get leaf nodes (host)
 */
BRTreeNode* ParallelBRTreeBuilder::get_leaf_nodes()
{
  copyFromGPUtoCPU((void**)&h_leaf_nodes, d_leaf_nodes, numLeafNode * sizeof(BRTreeNode));   
  return h_leaf_nodes;
}

/**
 * get internal nodes (host)
 */
BRTreeNode* ParallelBRTreeBuilder::get_internal_nodes()
{
  copyFromGPUtoCPU((void**)&h_internal_nodes, d_internal_nodes, numInternalNode * sizeof(BRTreeNode));   
  return h_internal_nodes;
}

/**
 * free memory on host
 */
void ParallelBRTreeBuilder::freeHostMemory()
{
   free(h_leaf_nodes);
   free(h_internal_nodes);
}

/**
 * free memory on device
 */
void ParallelBRTreeBuilder::freeDeviceMemory()
{
   cudaFree(d_leaf_nodes);
   cudaFree(d_internal_nodes);
   cudaFree(d_sorted_morton_code);
}

/**
 * deconstructor
 */
ParallelBRTreeBuilder::~ParallelBRTreeBuilder() {}


