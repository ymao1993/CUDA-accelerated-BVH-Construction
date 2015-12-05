#include "sampler.h"

#include "random_util.h"

namespace CMU462 {

// Uniform Sampler2D Implementation //

Vector2D UniformGridSampler2D::get_sample() const {
  return Vector2D(random_uniform(), random_uniform());
}

// Uniform Hemisphere Sampler3D Implementation //

// returns a random direction on the hemisphere
// (with uniform probability)
Vector3D UniformHemisphereSampler3D::get_sample() const {

  double cosTheta = random_uniform();
  double sinTheta = sqrt(1 - cosTheta * cosTheta);
  double phi = 2.0 * PI * random_uniform();

  double xs = sinTheta * cosf(phi);
  double ys = sinTheta * sinf(phi);
  double zs = cosTheta;

  return Vector3D(xs, ys, zs);
}

Vector3D CosineWeightedHemisphereSampler3D::get_sample() const {
  float f;
  return get_sample(&f);
}

// Samples on a disk and projects up to the hemisphere
Vector3D CosineWeightedHemisphereSampler3D::get_sample(float *pdf) const {
  double r = sqrt(random_uniform());
  double phi = 2 * PI * random_uniform();

  double x = r * cosf(phi);
  double y = r * sinf(phi);
  double z = sqrt(1 - x * x - y * y);
  *pdf = z / PI;
  return Vector3D(x, y, z);
}

} // namespace CMU462
