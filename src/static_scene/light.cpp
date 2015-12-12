#include "light.h"

#include <iostream>

#include "../sampler.h"

namespace CMU462 { namespace StaticScene {

// Directional Light //

DirectionalLight::DirectionalLight(const Spectrum& rad,
                                   const Vector3D& lightDir)
    : radiance(rad) {
  dirToLight = -lightDir.unit();
}

// ===RUI=== wi: dir_to_light (a vector)
Spectrum DirectionalLight::sample_L(const Vector3D& p, Vector3D* wi,
                                    float* distToLight, float* pdf) const {
  *wi = dirToLight;
  *distToLight = INF_D;
  *pdf = 1.0;
  return radiance;
}

// ===RUI=== wi: dir_to_light (a vector) =========TODO=========
// Spectrum DirectionalLight::sampleLight(Ray* lightRay, float* lightPdf) const {
//   lightRay->d = -dirToLight;
//   lightRay->o = 
//   *distToLight = INF_D;
//   *lightPdf = 1.0;
//   return radiance;
// }

// Infinite Hemisphere Light //

InfiniteHemisphereLight::InfiniteHemisphereLight(const Spectrum& rad)
    : radiance(rad) {
  sampleToWorld[0] = Vector3D(1,  0,  0);
  sampleToWorld[1] = Vector3D(0,  0, -1);
  sampleToWorld[2] = Vector3D(0,  1,  0);
}

Spectrum InfiniteHemisphereLight::sample_L(const Vector3D& p, Vector3D* wi,
                                           float* distToLight,
                                           float* pdf) const {
  *wi = sampleToWorld * sampler.get_sample();
  *distToLight = INF_D;
  *pdf = 1.0 / (2.0 * M_PI);
  return radiance;
}

// Point Light //

PointLight::PointLight(const Spectrum& rad, const Vector3D& pos) :
  radiance(rad), position(pos) { }

Spectrum PointLight::sample_L(const Vector3D& p, Vector3D* wi,
                             float* distToLight,
                             float* pdf) const {
  Vector3D d = position - p;
  double dist = d.norm();
  *wi = d / dist;
  *distToLight = dist;
  *pdf = 1.0;
  return radiance;
}

// Spot Light //

SpotLight::SpotLight(const Spectrum& rad, const Vector3D& pos,
                     const Vector3D& dir, float angle) {

}

// ===RUI=== never gonna shoot the light anyway
Spectrum SpotLight::sample_L(const Vector3D& p, Vector3D* wi,
                             float* distToLight, float* pdf) const {
  return Spectrum();
}


// Area Light //

AreaLight::AreaLight(const Spectrum& rad,
                     const Vector3D& pos,   const Vector3D& dir,
                     const Vector3D& dim_x, const Vector3D& dim_y)
  : radiance(rad), position(pos), direction(dir),
    dim_x(dim_x), dim_y(dim_y), area(dim_x.norm() * dim_y.norm()) { }


Spectrum AreaLight::sample_L(const Vector3D& p, Vector3D* wi,
                             float* distToLight, float* pdf) const {
  const Vector2D& sample = sampler.get_sample() - Vector2D(0.5f, 0.5f);
  const Vector3D& d = position + sample.x * dim_x + sample.y * dim_y - p;
  float cosTheta = dot(d, direction);
  float sqDist = d.norm2();
  float dist = sqrt(sqDist);
  *wi = d / dist;
  *distToLight = dist;
  *pdf = sqDist / (area * fabs(cosTheta));
  return cosTheta < 0 ? radiance : Spectrum();
};

// ===RUI=== 
Spectrum AreaLight::sampleLightFromP(const Vector3D& p, Vector3D& onLight, Vector3D& wi) const {
  const Vector2D& sample = sampler.get_sample() - Vector2D(0.5f, 0.5f);
  onLight = position + sample.x * dim_x + sample.y * dim_y;
  const Vector3D& d = position + sample.x * dim_x + sample.y * dim_y - p;
  float cosTheta = dot(d, direction);
  float sqDist = d.norm2();
  float dist = sqrt(sqDist);
  wi = d / dist;
  float pdf = sqDist / (area * fabs(cosTheta));
  return cosTheta < 0 ? 1.f / pdf * radiance : Spectrum();
};

// ===RUI=== NEW: get a light sample from the light
Spectrum AreaLight::sampleLight(Ray* lightRay, float* lightPdf) const {
  const Vector2D& sample = sampler.get_sample() - Vector2D(0.5f, 0.5f);
  const Vector3D& d = position + sample.x * dim_x + sample.y * dim_y;
  lightRay->o = d;
  float theta = ((double) rand() / (RAND_MAX)) * M_PI / 2.0;
  float phi = ((double) rand() / (RAND_MAX)) * M_PI * 2.0;

  UniformHemisphereSampler3D sampler;
  Vector3D localD = sampler.get_sample();
  Matrix3x3 w2o;
  make_coord_space(w2o, direction);
  Matrix3x3 o2w(w2o.T());
  lightRay->d = (o2w * localD).unit();

  *lightPdf = 1.0 / (area * 2.0 * M_PI);
  return radiance;
};

// Sphere Light //

SphereLight::SphereLight(const Spectrum& rad, const SphereObject* sphere) {

}

Spectrum SphereLight::sample_L(const Vector3D& p, Vector3D* wi,
                               float* distToLight, float* pdf) const {

  return Spectrum();
}

// Mesh Light

MeshLight::MeshLight(const Spectrum& rad, const Mesh* mesh) {

}

Spectrum MeshLight::sample_L(const Vector3D& p, Vector3D* wi,
                             float* distToLight, float* pdf) const {
  return Spectrum();
}

} // namespace StaticScene
} // namespace CMU462
