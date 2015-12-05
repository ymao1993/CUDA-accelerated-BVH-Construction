#include "sphere.h"

#include <cmath>

#include "../bsdf.h"
#include "../misc/sphere_drawing.h"

namespace CMU462 { namespace StaticScene {


bool Sphere::intersect(const Ray& r) const {

  double t1, t2;
  if (test(r, t1, t2)) {
      return
          (t2 >= r.min_t && t2 <= r.max_t) ||
          (t1 >= r.min_t && t1 <= r.max_t);
  }
  return false;
}

bool Sphere::intersect(const Ray& r, Intersection *isect) const {

  double t1, t2;
  if (test(r, t1, t2)) {

    double t = t1;
    if (t < r.min_t || t > r.max_t) t = t2;
    if (t < r.min_t || t > r.max_t) return false;

    r.max_t = t;
    isect->t = t;
    isect->n = normal(r.o + r.d * t);
    isect->primitive = this;
    isect->bsdf = object->get_bsdf();
    return true;
  }
  return false;
}

bool Sphere::test(const Ray& r, double& t1, double& t2) const {

  Vector3D s = o - r.o;
  double sd = dot(s, r.d);
  double ss = dot(s, s);

  // compute discriminant
  double disc = sd * sd - ss + r2;

  // complex values - no intersection
  if (disc < 0.0) return false;

  // check intersection time
  double sqrtDisc = sqrt(disc);
  t1 = sd - sqrtDisc;
  t2 = sd + sqrtDisc;

  return true;
}

void Sphere::draw(const Color& c) const {
  Misc::draw_sphere_opengl(o, r, c);
}

void Sphere::drawOutline(const Color& c) const {
    //Misc::draw_sphere_opengl(o, r, c);
}


} // namespace StaticScene
} // namespace CMU462
