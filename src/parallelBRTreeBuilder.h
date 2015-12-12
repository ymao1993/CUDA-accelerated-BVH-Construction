#ifndef PARALLELBRTREEBUILDER_H
#define PARALLELBRTREEBUILDER_H

#include <stdio.h>
#include <iostream>

#include "cuda_runtime.h"
#include "cuda.h"

#include "bbox.h"

using namespace CMU462;

/**
 * BRTreeNode
 *
 * BRTreeNode stands for a node in the 
 * binary radix tree. 
 *
 * the index of children and parent node
 * into the node array is encoded in the
 * following way: 
 *
 * 1) When the value is positive, it
 * refers to the node in internal node array.
 * the encoded value is (val-1)
 * 
 * 2) When the value is negative, it refers to
 * the node in leaf node array. And in the latter
 * situation, the encoded value is -(val+1)
 *
 * For example: If childA is 3, it means the left
 * child of the current node is in internal node
 * array with an offset of 2. If the childB is -1,
 * it means the right child of the current node
 * is in the leaf node array with an offset of 0.
 */

struct BRTreeNode
{
public:
  BRTreeNode():childA(0),childB(0),parent(0),idx(0),counter(0){}
   
  /*getters and setters for encoding and decoding*/
  __host__ __device__
  inline void setChildA(int _childA, bool is_leaf) 
  { if (is_leaf) { childA = -_childA - 1; } else{ childA = _childA + 1; } }
  
  __host__ __device__
  inline void setChildB(int _childB, bool is_leaf) 
  { if (is_leaf) { childB = -_childB - 1; } else{ childB = _childB + 1; } }
  
  __host__ __device__
  inline void setParent(int _parent) 
  { parent = _parent + 1; }  
  
  __host__ __device__
  inline void setIdx(int _idx) 
  { idx = _idx; }
  
  __host__ __device__
  inline int getChildA(bool& is_leaf, bool& is_null) 
  { if (childA == 0){ is_null = true; return -1; } is_null = false; is_leaf = childA < 0; if (is_leaf) return -(childA + 1); else return childA - 1; }
  
  __host__ __device__
  inline int getChildB(bool& is_leaf, bool& is_null) 
  { if (childB == 0){ is_null = true; return -1; } is_null = false; is_leaf = childB < 0; if (is_leaf) return -(childB + 1); else return childB - 1; }
  
  __host__ __device__
  inline int getParent(bool& is_null) 
  { if (parent == 0){ is_null = true; return -1; } is_null = false; return parent - 1; }  
  
  __host__ __device__
  inline int getIdx() {return idx;}
  
  __host__
  void printInfo()
  {
     bool is_leaf = false;
     bool is_null = false;
     int index = 0;
     
     printf("-----\n");
     index = getChildA(is_leaf, is_null);
     printf("childA:(%d,%d,%d)\n", index, is_leaf, !is_null);
     index = getChildB(is_leaf, is_null);
     printf("childB:(%d,%d,%d)\n", index, is_leaf, !is_null);
     index = getParent(is_null);
     printf("parent:(%d,%d)\n", index, !is_null);
     index = getIdx();
     printf("index:%d\n", index);
  }

public:
  unsigned int counter;
  BBox bbox;
  
private:
  int childA;
  int childB;
  int parent;
  int idx;
};

class ParallelBRTreeBuilder
{
public:
  ParallelBRTreeBuilder(unsigned int* const sorted_morton_code, BBox* const bboxes, int size);
  void build();
  
  BRTreeNode* get_leaf_nodes();
  BRTreeNode* get_internal_nodes();
  void freeHostMemory();
  void freeDeviceMemory();
  
  ~ParallelBRTreeBuilder();

  inline void printLeafNode()
  {
    for(int i=0; i<numLeafNode; i++)
    {
       h_leaf_nodes[i].printInfo();
    }
    return;
  } 

  inline void printInternalNode()
  {
    for(int i=0; i<numInternalNode; i++)
    {
       h_internal_nodes[i].printInfo();
    }
    return;
  } 

private:
  unsigned int* d_sorted_morton_code;
  BBox* d_bboxes;
  BRTreeNode* d_leaf_nodes;
  BRTreeNode* h_leaf_nodes;
  BRTreeNode* d_internal_nodes;
  BRTreeNode* h_internal_nodes;

  int numInternalNode;
  int numLeafNode;
  
};

#endif
