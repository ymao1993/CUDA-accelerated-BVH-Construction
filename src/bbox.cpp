#include "bbox.h"

#include "GL/glew.h"

#include <algorithm>
#include <iostream>

namespace CMU462 {

// Implementation based on Solid Angle's paper:
// Robust BVH Ray Traversal, Thiago Ize, Solid Angle
// Journal of Computer Graphics Techniques Vol. 2, No. 2, 2013

bool BBox::intersect(const Ray& r, double& t0, double& t1) const {

  Vector3D bounds[2] = { min, max };
  double txmin, txmax, tymin, tymax, tzmin, tzmax, tmax, tmin;

  txmin = (bounds[  r.sign[0]].x-r.o.x) * r.inv_d.x;
  txmax = (bounds[1-r.sign[0]].x-r.o.x) * r.inv_d.x;
  tymin = (bounds[  r.sign[1]].y-r.o.y) * r.inv_d.y;
  tymax = (bounds[1-r.sign[1]].y-r.o.y) * r.inv_d.y;
  tzmin = (bounds[  r.sign[2]].z-r.o.z) * r.inv_d.z;
  tzmax = (bounds[1-r.sign[2]].z-r.o.z) * r.inv_d.z;

  tmin = std::max(tzmin, std::max(tymin, std::max(txmin, t0)));
  tmax = std::min(tzmax, std::min(tymax, std::min(txmax, t1)));
  tmax *= 1.0000000000000004;

  if (tmin <= tmax) {
     t0 = tmin;
     t1 = tmax;
     return true;
  }

  return false;
}

void BBox::draw(Color c) const {

  glColor4f(c.r, c.g, c.b, c.a);

  // top
  glBegin(GL_LINE_STRIP);
  glVertex3d(max.x, max.y, max.z);
  glVertex3d(max.x, max.y, min.z);
  glVertex3d(min.x, max.y, min.z);
  glVertex3d(min.x, max.y, max.z);
  glVertex3d(max.x, max.y, max.z);
  glEnd();

  // bottom
  glBegin(GL_LINE_STRIP);
  glVertex3d(min.x, min.y, min.z);
  glVertex3d(min.x, min.y, max.z);
  glVertex3d(max.x, min.y, max.z);
  glVertex3d(max.x, min.y, min.z);
  glVertex3d(min.x, min.y, min.z);
  glEnd();

  // side
  glBegin(GL_LINES);
  glVertex3d(max.x, max.y, max.z);
  glVertex3d(max.x, min.y, max.z);
  glVertex3d(max.x, max.y, min.z);
  glVertex3d(max.x, min.y, min.z);
  glVertex3d(min.x, max.y, min.z);
  glVertex3d(min.x, min.y, min.z);
  glVertex3d(min.x, max.y, max.z);
  glVertex3d(min.x, min.y, max.z);
  glEnd();

}

std::ostream& operator<<(std::ostream& os, const BBox& b) {
  return os << "BBOX(" << b.min << ", " << b.max << ")";
}

} // namespace CMU462
