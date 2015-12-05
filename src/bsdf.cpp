#include "bsdf.h"

#include <iostream>
#include <algorithm>
#include <utility>

#include "random_util.h"

using std::min;
using std::max;
using std::swap;

namespace CMU462 {

void make_coord_space(Matrix3x3& o2w, const Vector3D& n) {

    Vector3D z = Vector3D(n.x, n.y, n.z);
    Vector3D h = z;
    if (fabs(h.x) <= fabs(h.y) && fabs(h.x) <= fabs(h.z)) h.x = 1.0;
    else if (fabs(h.y) <= fabs(h.x) && fabs(h.y) <= fabs(h.z)) h.y = 1.0;
    else h.z = 1.0;

    z.normalize();
    Vector3D y = cross(h, z);
    y.normalize();
    Vector3D x = cross(z, y);
    x.normalize();

    o2w[0] = x;
    o2w[1] = y;
    o2w[2] = z;
}

// Diffuse BSDF //

Spectrum DiffuseBSDF::f(const Vector3D& wo, const Vector3D& wi) {
  return albedo * (1.0 / PI);
}

Spectrum DiffuseBSDF::sample_f(const Vector3D& wo, Vector3D* wi, float* pdf) {
  *wi = sampler.get_sample(pdf);
  return albedo * (1.0 / PI);
}

// Mirror BSDF //

Spectrum MirrorBSDF::f(const Vector3D& wo, const Vector3D& wi) {
  return Spectrum();
}

Spectrum MirrorBSDF::sample_f(const Vector3D& wo, Vector3D* wi, float* pdf) {
  *pdf = 1.0f;
  reflect(wo, wi);
  return (1.f / cos_theta(wo)) * reflectance;
}

// Glossy BSDF //

/*
Spectrum GlossyBSDF::f(const Vector3D& wo, const Vector3D& wi) {
  return Spectrum();
}

Spectrum GlossyBSDF::sample_f(const Vector3D& wo, Vector3D* wi,
                              float* pdf) {
  *pdf = 1.0f;
  return reflect(wo, wi, reflectance);
}
*/

// Refraction BSDF //

Spectrum RefractionBSDF::f(const Vector3D& wo, const Vector3D& wi) {
  return Spectrum();
}

Spectrum RefractionBSDF::sample_f(const Vector3D& wo, Vector3D* wi, float* pdf) {

  *pdf = 1.0f;

  float cosTheta = cos_theta(wo);
  bool entering = cosTheta > 0;
  float ei = 1.f, et = ior;
  if (!entering) {
      swap(ei, et);
      cosTheta = -cosTheta;
  }
  float inveta = et / ei;
  float inveta2 = inveta * inveta;

  if (refract(wo, wi, ior))
      return inveta2 / cosTheta * transmittance;
  else
      return Spectrum();  // total internal reflection case
}

// Glass BSDF //

Spectrum GlassBSDF::f(const Vector3D& wo, const Vector3D& wi) {
  // assume that wi IS NEVER the perfect mirror reflection of wo
  // about the surface normal.
  return Spectrum();
}

Spectrum GlassBSDF::sample_f(const Vector3D& wo, Vector3D* wi, float* pdf) {

  // compute Fresnel coefficient and use it as the probability of reflection
  // - Fundamentals of Computer Graphics page 305
  float R0 = (ior-1.0)*(ior-1.0)/((ior + 1.0)*(ior + 1.0));
  float cosTheta = cos_theta(wo);
  float f = 1 - fabs(cosTheta);
  float g = ((f * f) * (f * f)) * f;
  float fresnel_coe = R0 + (1.0 - R0)*g;

  bool entering = cos_theta(wo) > 0;
  float ei = 1.f, et = ior;
  if (!entering) {
      swap(ei, et);
      cosTheta = -cosTheta;  // be careful here, want cosTheta to be
                             // positive for everything below
  }
  float inveta = et / ei;
  float inveta2 = inveta * inveta;

  if (!refract(wo, wi, ior)) {
    // total internal reflection; always reflect
    *pdf = 1.0;
    reflect(wo, wi);
    return (1 / cosTheta) * reflectance;
  }

  if (coin_flip(fresnel_coe)) {
    *pdf = fresnel_coe;
    reflect(wo, wi);
    return (fresnel_coe / cosTheta) * reflectance;
  } else {
    // refraction ray has already been computed
    float one_minus_fresnel = 1.0f - fresnel_coe;
    *pdf = one_minus_fresnel;
    return (one_minus_fresnel * inveta2 / cosTheta) * transmittance;
  }
}

void BSDF::reflect(const Vector3D& wo, Vector3D* wi) {
  *wi = Vector3D(-wo.x, -wo.y, wo.z);
}

// return true if refraction occurs, return false in total internal
// reflection case
bool BSDF::refract(const Vector3D& wo, Vector3D* wi, float ior) {

  bool entering = cos_theta(wo) > 0;

  float ei = 1.f, et = ior;
  if (!entering) swap(ei, et);

  float sini2 = sin_theta2(wo);
  float eta = ei / et;
  float sint2 = eta * eta * sini2;
  if (sint2 > 1.f) return false;
  float cost = sqrt(1.0f - sint2);

  if (entering) cost = -cost;
  float sint_over_sini = eta;

  *wi = Vector3D(-sint_over_sini * wo.x, -sint_over_sini * wo.y, cost);

  return true;
}

// Emission BSDF //

Spectrum EmissionBSDF::f(const Vector3D& wo, const Vector3D& wi) {
  return Spectrum();
}

Spectrum EmissionBSDF::sample_f(const Vector3D& wo, Vector3D* wi, float* pdf) {
  *pdf = 1.0 / PI;
  *wi  = sampler.get_sample(pdf);
  return Spectrum();
}

} // namespace CMU462
