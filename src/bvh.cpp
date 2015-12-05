#include "bvh.h"

#include "CMU462/CMU462.h"
#include "static_scene/triangle.h"

#include <iostream>
#include <stack>

using namespace std;

namespace CMU462 { namespace StaticScene {

static const size_t kMaxNumBuckets = 12;

struct BVHBuildData {
  BVHBuildData(BBox bb, size_t start, size_t range, BVHNode **dst)
      : bb(bb), start(start), range(range), node(dst) {}
  BBox bb;         ///< bounding box enclosing the primitives
  size_t start;    ///< start index into the primitive array
  size_t range;    ///< range of index into the primitive array
  BVHNode **node;  ///< address to store new node address
};

struct SAHBucketData {
  SAHBucketData() {}
  BBox bb;           ///< bbox of all primitives
  size_t num_prims;  ///< number of primitives in the bucket
};

BVHAccel::BVHAccel(const std::vector<Primitive *> &_primitives,
                   size_t max_leaf_size) {
  this->primitives = _primitives;

  // create build stack
  stack<BVHBuildData> bstack;

  // edge case
  if (primitives.empty()) {
    return;
  }

  // create initial build data
  BBox bb;
  for (size_t i = 0; i < primitives.size(); ++i) {
    bb.expand(primitives[i]->get_bbox());
  }

  // push initial build data (build root)
  bstack.push(BVHBuildData(bb, 0, primitives.size(), &root));

  // build loop
  while (!bstack.empty()) {

    // pop work form build stack
    BVHBuildData bdata = bstack.top();
    bstack.pop();

    // create new node
    *bdata.node = new BVHNode(bdata.bb, bdata.start, bdata.range);

    // done if leaf, or fewer primitives than buckets
    if (bdata.range <= max_leaf_size) {
      continue;
    }

    // allocate SAH evaluation buckets for splits
    size_t num_buckets = std::min(kMaxNumBuckets, bdata.range);
    SAHBucketData* buckets = new SAHBucketData[num_buckets];

    // compute sah cost for all three possible split directions
    // with a default being the cost of doing no split.
    BBox split_Ba;
    BBox split_Bb;
    int split_dim = -1;
    size_t split_idx = 1;
    double split_val = 0;
    double split_cost = bdata.bb.surface_area() * bdata.range;

    // try all three dimensions and find best split
    for (int dim = 0; dim < 3; dim++) {
      if (bdata.bb.extent[dim] < EPS_D) {
        continue; // ignore flat dimension
      }

      // initialize SAH evaluation buckets
      double bucket_width = bdata.bb.extent[dim] / num_buckets;
      for (size_t i = 0; i < num_buckets; ++i) {
        buckets[i].bb = BBox();
        buckets[i].num_prims = 0;
      }

      // compute bucket bboxes
      for (size_t i = bdata.start; i < bdata.start + bdata.range; ++i) {
        Primitive *p = primitives[i];
        BBox pbb = p->get_bbox();
        double d = (pbb.centroid()[dim] - bdata.bb.min[dim]) / bucket_width;
        size_t b = clamp((int)d, 0, ((int)num_buckets) - 1);
        buckets[b].bb.expand(pbb);
        buckets[b].num_prims++;
      }

      // evaluation split costs
      for (size_t idx = 1; idx < num_buckets; ++idx) {
        // Sa * Na
        size_t Na = 0;
        BBox Ba;
        for (size_t i = 0; i < idx; ++i) {
          Ba.expand(buckets[i].bb);
          Na += buckets[i].num_prims;
        }

        // Sb * Nb
        size_t Nb = 0;
        BBox Bb;
        for (size_t i = idx; i < num_buckets; ++i) {
          Bb.expand(buckets[i].bb);
          Nb += buckets[i].num_prims;
        }

        // sah cost & actual split value
        double cost = Na * Ba.surface_area() + Nb * Bb.surface_area();
        double val = bdata.bb.min[dim] + idx * bucket_width;
        if (cost < split_cost) {
          split_Ba = Ba;
          split_Bb = Bb;
          split_dim = dim;
          split_idx = idx;
          split_val = val;
          split_cost = cost;
        }
      }
    }

    // done with buckets
    delete buckets;

    // edge case - if all dimensions are flat (all centroids on a single spot)
    // split equally into two build nodes with the same bbox
    if (split_dim == -1) {
      size_t startl = bdata.start;
      size_t rangel = bdata.range / 2;
      size_t startr = startl + rangel;
      size_t ranger = bdata.range - rangel;
      bstack.push(BVHBuildData(bdata.bb, startl, rangel, &(*bdata.node)->l));
      bstack.push(BVHBuildData(bdata.bb, startr, ranger, &(*bdata.node)->r));
      continue;
    }

    // partition primitives
    std::vector<Primitive *>::iterator it =
        std::partition(primitives.begin() + bdata.start,
                       primitives.begin() + bdata.start + bdata.range,
                       [split_dim, split_val](Primitive *p) {
                         return p->get_bbox().centroid()[split_dim] < split_val;
                       });

    size_t startl = bdata.start;
    size_t rangel = distance(primitives.begin(),it) - bdata.start;
    size_t startr = startl + rangel;
    size_t ranger = bdata.range - rangel;

    // create new build data
    bstack.push(BVHBuildData(split_Ba, startl, rangel, &(*bdata.node)->l));
    bstack.push(BVHBuildData(split_Bb, startr, ranger, &(*bdata.node)->r));
  }
}

static void rec_free(BVHNode *node) {
  if (node->l) rec_free(node->l);
  if (node->r) rec_free(node->r);
  delete node;
}

BVHAccel::~BVHAccel() { rec_free(root); }

BBox BVHAccel::get_bbox() const { return root->bb; }

bool BVHAccel::intersect(const Ray &ray) const {

  double t0 = ray.min_t;
  double t1 = ray.max_t;

  // try early exit
  if (!root->bb.intersect(ray, t0, t1)) return false;

  // create traversal stack
  stack<BVHNode *> tstack;

  // push initial traversal data
  tstack.push(root);

  // process traversal
  BVHNode *l, *r;
  while (!tstack.empty()) {

    // pop traversal data
    BVHNode *current = tstack.top();
    tstack.pop();

    // get children
    l = current->l;
    r = current->r;

    // if leaf
    if (!(l || r)) {
      for (size_t i = 0; i < current->range; ++i) {
        if (primitives[current->start + i]->intersect(ray)) return true;
      }
    }

    // test bboxes
    double tl0 = ray.min_t;
    double tl1 = ray.max_t;
    double tr0 = ray.min_t;
    double tr1 = ray.max_t;
    bool hitL, hitR;
    hitL = (l != NULL) && l->bb.intersect(ray, tl0, tl1);
    hitR = (r != NULL) && r->bb.intersect(ray, tr0, tr1);

    // both hit
    if (hitL && hitR) {
      tstack.push(l);
      tstack.push(r);
    } else if (hitL) {
      tstack.push(l);
    } else if (hitR) {
      tstack.push(r);
    }
  }

  return false;
}

bool BVHAccel::intersect(const Ray &ray, Intersection *isect) const {

  bool hit = false;  // never leave such things uninitialized :D

  double t0 = ray.min_t;
  double t1 = ray.max_t;

  // try early exit
  if (!root->bb.intersect(ray, t0, t1)) return false;

  // create traversal stack
  stack<BVHNode *> tstack;

  // push initial traversal data
  tstack.push(root);

  // process traversal
  while (!tstack.empty()) {

    // pop traversal data
    BVHNode *current = tstack.top();
    tstack.pop();

    // get childrren
    BVHNode *l, *r;
    l = current->l;
    r = current->r;

    // if leaf
    if (!(l || r)) {
      for (size_t p = 0; p < current->range; ++p) {
        if (primitives[current->start + p]->intersect(ray, isect)) hit = true;
      }
    }

    // bbox test
    double tl0 = ray.min_t;
    double tl1 = ray.max_t;
    double tr0 = ray.min_t;
    double tr1 = ray.max_t;
    bool hitL, hitR;
    hitL = (l != NULL) && l->bb.intersect(ray, tl0, tl1);
    hitR = (r != NULL) && r->bb.intersect(ray, tr0, tr1);

    if (hitL && hitR) {
      tstack.push(r);
      tstack.push(l);
    } else if (hitL) {
      tstack.push(l);
    } else if (hitR) {
      tstack.push(r);
    }

  }

  return hit;
}

}  // namespace StaticScene
}  // namespace CMU462
