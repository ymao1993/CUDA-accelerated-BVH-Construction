#ifndef CMU462_STATICSCENE_ENVIRONMENTLIGHT_H
#define CMU462_STATICSCENE_ENVIRONMENTLIGHT_H

#include "../sampler.h"
#include "../image.h"
#include "alias_table.h"
#include "scene.h"

namespace CMU462 { namespace StaticScene {

class EnvironmentLight : public SceneLight {
 public:
  EnvironmentLight(const HDRImageBuffer* envMap);
  ~EnvironmentLight();
  Spectrum sample_L(const Vector3D& p, Vector3D* wi, float* distToLight,
                    float* pdf) const;
  bool is_delta_light() const { return false; }
  Spectrum sample_dir(const Ray& r) const;
 
 private:
  const HDRImageBuffer* envMap;
  // worldToLocal rotates by -pi/2 about y axis, localToWorld does the inverse.
  Matrix3x3 localToWorld, worldToLocal;
  // Colors at the top and bottom of the scene, for use in wraparound.
  Spectrum topL, bottomL;

  AliasTable *aliasTable; // for lookups
  float *cellAreas; // cellAreas[y] = SA of cell in row y
}; // class EnvironmentLight

} // namespace StaticScene
} // namespace CMU462

#endif //CMU462_STATICSCENE_ENVIRONMENTLIGHT_H
