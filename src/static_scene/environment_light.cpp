#include "environment_light.h"

#include "../random_util.h"

#include <iostream>

using std::cout;
using std::endl;

namespace CMU462 { namespace StaticScene {

EnvironmentLight::EnvironmentLight(const HDRImageBuffer* envMap)
  : envMap(envMap) {
  worldToLocal[0] = Vector3D(0, 0, 1);
  worldToLocal[1] = Vector3D(0, 1, 0);
  worldToLocal[2] = Vector3D(-1, 0, 0);
  localToWorld = worldToLocal.T();
  
  // Generate colors for the top and bottom of the screen, to avoid strange
  // wraparound cases in bilinear filtering.
  topL = Spectrum();
  bottomL = Spectrum();
  int lastRowStart = (envMap->h - 1) * envMap->w;
  for (int x = 0; x < envMap->w; x++) {
    topL += envMap->data[x];
    bottomL += envMap->data[lastRowStart + x];
  }
  topL *= (1.0 / envMap->w);
  bottomL *= (1.0 / envMap->w);

  // Generate sample probabilities.
  const size_t w = envMap->w;
  const size_t h = envMap->h;
  double *probs = new double[w * h];
  cellAreas = new float[h];
  const Spectrum *data = envMap->data.data();

  double netSum = 0;
  for (int y = 0; y < h; y++) {
    float theta1 = ((float)y) / h * M_PI;
    float theta2 = ((float)(y + 1)) / h * M_PI;
    float dPhi = 2 * M_PI / w;
    float cellArea = dPhi * (cos(theta1) - cos(theta2));
    cellAreas[y] = cellArea;
    for (int i = y * w; i < y * w + w; i++) {
      probs[i] = data[i].illum() * cellArea;
      netSum += probs[i];
    }
  }
  // Scale so the average value is 1 (for error reducing reasons).
  for (int i = 0; i < w * h; i++) {
    probs[i] *= (w * h / netSum);
  }
  aliasTable = new AliasTable(probs, w * h);
}

EnvironmentLight::~EnvironmentLight() {
  delete aliasTable;
  delete cellAreas;
}

// We choose samples from among the pixel centers of our environment map; this
// drastically simplifies sampling and sacrifices little in terms of quality
// since the environment map is usually quite big.
Spectrum EnvironmentLight::sample_L(const Vector3D& p, Vector3D* wi,
                                    float* distToLight,
                                    float* pdf) const {
  size_t w = envMap->w;
  size_t h = envMap->h;
  
  size_t i = aliasTable->sample(pdf);
  size_t y = i / w;
  size_t x = i % w; 

  float phi = (x + 0.5)/w * (2 * PI);
  float theta = (y + 0.5)/h * PI;
  float sinTheta = sinf(theta);
  Vector3D dir(sinTheta * cosf(phi), cosf(theta), sinTheta * sinf(phi));
  *wi = localToWorld * dir;
  *distToLight = INF_F;
  return envMap->data[i] * cellAreas[y];
}

// Unlike sample_L, the ray could point at any arbitrary point instead of just
// pixel centers. We use bilinear interpolation to sample more smoothly.
Spectrum EnvironmentLight::sample_dir(const Ray& r) const {
  const Vector3D& dir = (worldToLocal * r.d).unit();
  // compute UV
  float u = atan2(dir.z, dir.x) / (2 * M_PI) + 0.5f;
  float v = acos(dir.y) / M_PI;

  const size_t w = envMap->w;
  const size_t h = envMap->h;

  // bilerp
  // Adding an extra w to x to make the wraparound code cleaner.
  float x = u * w + w;
  float y = v * h;

  size_t x0 = floorf(x - 0.5f);
  size_t x1 = (x0 + 1);
  float wx0 = (x1 + 0.5) - x;
  float wx1 = x - (x0 + 0.5);
  x0 %= w;
  x1 %= w;

  if (y < 0.5) {
    float wy1 = 0.5 + y;
    return envMap->data[x0] * (wx0 * wy1) +
           envMap->data[x1] * (wx1 * wy1) +
           topL * (1 - wy1);
  } else if (y > h - 0.5) {
    float wy0 = h + 0.5 - y;
    return envMap->data[x0 + (h - 1) * w] * (wx0 * wy0) +
           envMap->data[x1 + (h - 1) * w] * (wx1 * wy0) +
           bottomL * (1 - wy0);
  } else {
    size_t y0 = floorf(y - 0.5f);
    size_t y1 = (y0 + 1);
    float wy0 = (y1 + 0.5) - y;
    float wy1 = 1 - wy0;
    return envMap->data[x0 + y0 * w] * wx0 * wy0 +
           envMap->data[x0 + y1 * w] * wx0 * wy1 +
           envMap->data[x1 + y0 * w] * wx1 * wy0 +
           envMap->data[x1 + y1 * w] * wx1 * wy1;
  }
}

} // namespace StaticScene
} // namespace CMU462
