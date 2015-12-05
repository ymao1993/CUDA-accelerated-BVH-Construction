#include "triangle.h"

#include "CMU462/CMU462.h"
#include "GL/glew.h"

namespace CMU462 { namespace StaticScene {

bool intersect_triangle(const Ray& r,
    const Vector3D& a, const Vector3D& b, const Vector3D& c,
    double& alphaR, double& betaR, double& gammaR, double& tR) {
  // Find ray parameter t for intersection with plane of triangle
  const Vector3D& v0 = b - a;
  const Vector3D& v1 = c - a;
  const Vector3D& n = cross(v0, v1).unit();
  const double t = dot(a - r.o, n) / dot(r.d, n);
  if (t < r.min_t || t > r.max_t) {
    return false;
  }

  // Find barycentric coordinates, and return false if they're out of range
  const double d00 = dot(v0, v0);
  const double d01 = dot(v0, v1);
  const double d11 = dot(v1, v1);
  const Vector3D& v2 = r.at_time(t) - a;
  const double d20 = dot(v2, v0);
  const double d21 = dot(v2, v1);
  const double invDenom = 1.0 / (d00 * d11 - d01 * d01);
  const double beta = (d11 * d20 - d01 * d21) * invDenom;
  if (beta < 0.0 || beta > 1.0) {
    return false;
  }
  const double gamma = (d00 * d21 - d01 * d20) * invDenom;
  if (gamma < 0.0 || gamma > 1.0 - beta) {
    return false;
  }

  alphaR = 1.0 - beta - gamma;
  betaR = beta;
  gammaR = gamma;
  tR = t;
  return true;
}

Triangle::Triangle(const Mesh* mesh, size_t v1, size_t v2, size_t v3) :
    mesh(mesh), v1(v1), v2(v2), v3(v3) { }

BBox Triangle::get_bbox() const {
  BBox bbox(mesh->positions[v1]);
  bbox.expand(mesh->positions[v2]);
  bbox.expand(mesh->positions[v3]);
  return bbox;
}

bool Triangle::intersect(const Ray& r) const {
  double alpha, beta, gamma, t;
  return intersect_triangle(r,
                            mesh->positions[v1],
                            mesh->positions[v2],
                            mesh->positions[v3],
                            alpha, beta, gamma, t);
}

bool Triangle::intersect(const Ray& r, Intersection *isect) const {

  double alpha, beta, gamma, t;
  Vector3D a = mesh->positions[v1];
  Vector3D b = mesh->positions[v2];
  Vector3D c = mesh->positions[v3];

  if (intersect_triangle(r, a, b, c, alpha, beta, gamma, t)) {

    // interpolate normal
    Vector3D n = alpha * mesh->normals[v1] +
        beta  * mesh->normals[v2] +
        gamma * mesh->normals[v3];

    r.max_t = t;
    isect->t = t;

    // if we hixt the back of a triangle, we want to flip the normal so
    // the shading normal is pointing toward the incoming ray
    if (dot(n, r.d) > 0)
        isect->n = -n;
    else
        isect->n = n;

    isect->primitive = this;
    isect->bsdf = mesh->get_bsdf();

    return true;
  }
  return false;
}

void Triangle::draw(const Color& c) const {
  glColor4f(c.r, c.g, c.b, c.a);
  glBegin(GL_TRIANGLES);
  glVertex3d(mesh->positions[v1].x,
             mesh->positions[v1].y,
             mesh->positions[v1].z);
  glVertex3d(mesh->positions[v2].x,
             mesh->positions[v2].y,
             mesh->positions[v2].z);
  glVertex3d(mesh->positions[v3].x,
             mesh->positions[v3].y,
             mesh->positions[v3].z);
  glEnd();
}

void Triangle::drawOutline(const Color& c) const {
  glColor4f(c.r, c.g, c.b, c.a);
  glBegin(GL_LINE_LOOP);
  glVertex3d(mesh->positions[v1].x,
             mesh->positions[v1].y,
             mesh->positions[v1].z);
  glVertex3d(mesh->positions[v2].x,
             mesh->positions[v2].y,
             mesh->positions[v2].z);
  glVertex3d(mesh->positions[v3].x,
             mesh->positions[v3].y,
             mesh->positions[v3].z);
  glEnd();
}



} // namespace StaticScene
} // namespace CMU462
