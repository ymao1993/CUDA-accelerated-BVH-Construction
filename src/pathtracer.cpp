// The Official Reference Code

#include "pathtracer.h"
#include "bsdf.h"
#include "ray.h"

#include <stack>
#include <random>
#include <algorithm>

#include "CMU462/CMU462.h"
#include "CMU462/vector3D.h"
#include "CMU462/matrix3x3.h"
#include "CMU462/lodepng.h"

#include "GL/glew.h"

#include "random_util.h"

#include "static_scene/sphere.h"
#include "static_scene/triangle.h"
#include "static_scene/light.h"

#include <cassert>

using namespace CMU462::StaticScene;

using std::min;
using std::max;

namespace CMU462 {

//#define ENABLE_RAY_LOGGING 1
//#define ENABLE_RAY_TEST

//#define ENABLE_PATH_TRACING /* Quote this to enable Bidirectional Path Tracing; Unquote this to use classic path tracing */

PathTracer::PathTracer(size_t ns_aa,
                       size_t max_ray_depth, size_t ns_area_light,
                       size_t ns_diff, size_t ns_glsy, size_t ns_refr,
                       size_t num_threads, HDRImageBuffer* envmap, size_t ifBDPT) 
{
  state = INIT,
  this->ns_aa = ns_aa;
  set_sample_pattern();
  this->max_ray_depth = max_ray_depth;
  this->ns_area_light = ns_area_light;
  this->ns_diff = ns_diff;
  this->ns_glsy = ns_diff;
  this->ns_refr = ns_refr;
  this->useBDPT = ifBDPT;
  cout<<"this->useBDPT"<<this->useBDPT<<endl;

  if (envmap) {
    this->envLight = new EnvironmentLight(envmap);
  } else {
    this->envLight = NULL;
  }

  bvh = NULL;
  scene = NULL;
  camera = NULL;

  gridSampler = new UniformGridSampler2D();
  hemisphereSampler = new UniformHemisphereSampler3D();

  show_rays = true;

  imageTileSize = 32;
  numWorkerThreads = num_threads;
  workerThreads.resize(numWorkerThreads);

  tm_gamma = 2.2f;
  tm_level = 1.0f;
  tm_key = 0.18;
  tm_wht = 5.0f;

}

PathTracer::~PathTracer() {

  delete bvh;
  delete gridSampler;
  delete hemisphereSampler;

}

void PathTracer::set_scene(Scene *scene) {

  if (state != INIT) {
    return;
  }

  if (this->scene != nullptr) {
    delete scene;
    delete bvh;
    selectionHistory.pop();
  }

  if (this->envLight != nullptr) {
    scene->lights.push_back(this->envLight);
  }

  this->scene = scene;
  build_accel();

  if (has_valid_configuration()) {
    state = READY;
  }
}

void PathTracer::set_camera(Camera *camera) {

  if (state != INIT) {
    return;
  }

  this->camera = camera;
  if (has_valid_configuration()) {
    state = READY;
  }

}

void PathTracer::set_frame_size(size_t width, size_t height) {
  if (state != INIT && state != READY) {
    stop();
  }
  sampleBuffer.resize(width, height);
  frameBuffer.resize(width, height);
  if (has_valid_configuration()) {
    state = READY;
  }
}

bool PathTracer::has_valid_configuration() {
  return scene && camera && gridSampler && hemisphereSampler &&
         (!sampleBuffer.is_empty());
}

void PathTracer::update_screen() {
  switch (state) {
    case INIT:
    case READY:
      break;
    case VISUALIZE:
      visualize_accel();
      break;
    case RENDERING:
      glDrawPixels(frameBuffer.w, frameBuffer.h, GL_RGBA,
                   GL_UNSIGNED_BYTE, &frameBuffer.data[0]);
      break;
    case DONE:
        //sampleBuffer.tonemap(frameBuffer, tm_gamma, tm_level, tm_key, tm_wht);
      glDrawPixels(frameBuffer.w, frameBuffer.h, GL_RGBA,
                   GL_UNSIGNED_BYTE, &frameBuffer.data[0]);
      break;
  }
}

void PathTracer::stop() {
  switch (state) {
    case INIT:
    case READY:
      break;
    case VISUALIZE:
      while (selectionHistory.size() > 1) {
        selectionHistory.pop();
      }
      state = READY;
      break;
    case RENDERING:
      continueRaytracing = false;
    case DONE:
      for (int i=0; i<numWorkerThreads; i++) {
            workerThreads[i]->join();
            delete workerThreads[i];
        }
      state = READY;
      break;
  }
}

void PathTracer::clear() {
  if (state != READY) return;
  delete bvh;
  bvh = NULL;
  scene = NULL;
  camera = NULL;
  selectionHistory.pop();
  sampleBuffer.resize(0, 0);
  frameBuffer.resize(0, 0);
  state = INIT;
}

void PathTracer::start_visualizing() {
  if (state != READY) {
    return;
  }
  state = VISUALIZE;
}

void PathTracer::start_raytracing() {
  if (state != READY) return;

  rayLog.clear();
  workQueue.clear();

  state = RENDERING;
  continueRaytracing = true;
  workerDoneCount = 0;

  sampleBuffer.clear();
  frameBuffer.clear();
  num_tiles_w = sampleBuffer.w / imageTileSize + 1;
  num_tiles_h = sampleBuffer.h / imageTileSize + 1;
  tile_samples.resize(num_tiles_w * num_tiles_h);
  memset(&tile_samples[0], 0, num_tiles_w * num_tiles_h * sizeof(int));

  // populate the tile work queue
  for (size_t y = 0; y < sampleBuffer.h; y += imageTileSize) {
      for (size_t x = 0; x < sampleBuffer.w; x += imageTileSize) {
          workQueue.put_work(WorkItem(x, y, imageTileSize, imageTileSize));
      }
  }

  // launch threads
  fprintf(stdout, "[PathTracer] Rendering... "); fflush(stdout);
  for (int i=0; i<numWorkerThreads; i++) {
      workerThreads[i] = new std::thread(&PathTracer::worker_thread, this);
  }
}


void PathTracer::build_accel() {

  // collect primitives //
  fprintf(stdout, "[PathTracer] Collecting primitives... "); fflush(stdout);
  timer.start();
  vector<Primitive *> primitives;
  for (SceneObject *obj : scene->objects) {
    const vector<Primitive *> &obj_prims = obj->get_primitives();
    primitives.reserve(primitives.size() + obj_prims.size());
    primitives.insert(primitives.end(), obj_prims.begin(), obj_prims.end());
  }
  timer.stop();
  fprintf(stdout, "Done! (%.4f sec)\n", timer.duration());

  // build BVH //
  fprintf(stdout, "[PathTracer] Building BVH... "); fflush(stdout);
  timer.start();
  bvh = new BVHAccel(primitives);
  timer.stop();
  fprintf(stdout, "Done! (%.4f sec)\n", timer.duration());

  // initial visualization //
  selectionHistory.push(bvh->get_root());
}

void PathTracer::log_ray_miss(const Ray& r) {
    rayLog.push_back(LoggedRay(r, -1.0));
}

void PathTracer::log_ray_hit(const Ray& r, double hit_t) {
    rayLog.push_back(LoggedRay(r, hit_t));
}

void PathTracer::visualize_accel() const {

  glPushAttrib(GL_ENABLE_BIT);
  glDisable(GL_LIGHTING);
  glLineWidth(1);
  glEnable(GL_DEPTH_TEST);

  // hardcoded color settings
  Color cnode = Color(.5, .5, .5, .25);
  Color cnode_hl = Color(1., .25, .0, .6);
  Color cnode_hl_child = Color(1., 1., 1., .6);

  Color cprim_hl_left = Color(.6, .6, 1., 1);
  Color cprim_hl_right = Color(.8, .8, 1., 1);
  Color cprim_hl_edges = Color(0., 0., 0., 0.5);

  BVHNode *selected = selectionHistory.top();

  // render solid geometry (with depth offset)
  glPolygonOffset(1.0, 1.0);
  glEnable(GL_POLYGON_OFFSET_FILL);

  if (selected->isLeaf()) {
    for (size_t i = 0; i < selected->range; ++i) {
       bvh->primitives[selected->start + i]->draw(cprim_hl_left);
    }
  } else {
      if (selected->l) {
          BVHNode* child = selected->l;
          for (size_t i = 0; i < child->range; ++i) {
              bvh->primitives[child->start + i]->draw(cprim_hl_left);
          }
      }
      if (selected->r) {
          BVHNode* child = selected->r;
          for (size_t i = 0; i < child->range; ++i) {
              bvh->primitives[child->start + i]->draw(cprim_hl_right);
          }
      }
  }

  glDisable(GL_POLYGON_OFFSET_FILL);

  // draw geometry outline
  for (size_t i = 0; i < selected->range; ++i) {
      bvh->primitives[selected->start + i]->drawOutline(cprim_hl_edges);
  }

  // keep depth buffer check enabled so that mesh occluded bboxes, but
  // disable depth write so that bboxes don't occlude each other.
  glDepthMask(GL_FALSE);

  // create traversal stack
  stack<BVHNode *> tstack;

  // push initial traversal data
  tstack.push(bvh->get_root());

  // draw all BVH bboxes with non-highlighted color
  while (!tstack.empty()) {

    BVHNode *current = tstack.top();
    tstack.pop();

    current->bb.draw(cnode);
    if (current->l) tstack.push(current->l);
    if (current->r) tstack.push(current->r);
  }

  // draw selected node bbox and primitives
  if (selected->l) selected->l->bb.draw(cnode_hl_child);
  if (selected->r) selected->r->bb.draw(cnode_hl_child);

  glLineWidth(3.f);
  selected->bb.draw(cnode_hl);

  // now perform visualization of the rays
  if (show_rays) {
      glLineWidth(1.f);
      glBegin(GL_LINES);

      for (size_t i=0; i<rayLog.size(); i+=500) {

          const static double VERY_LONG = 10e4;
          double ray_t = VERY_LONG;

          // color rays that are hits yellow
          // and rays this miss all geometry red
          if (rayLog[i].hit_t >= 0.0) {
              ray_t = rayLog[i].hit_t;
              glColor4f(0.f, 0.f, 0.f, 0.1f);
          } else {
              glColor4f(1.f, 0.f, 0.f, 0.1f);
          }

          Vector3D end = rayLog[i].o + ray_t * rayLog[i].d;

          glVertex3f(rayLog[i].o[0], rayLog[i].o[1], rayLog[i].o[2]);
          glVertex3f(end[0], end[1], end[2]);
      }
      glEnd();
  }

  glDepthMask(GL_TRUE);
  glPopAttrib();
}

void PathTracer::key_press(int key) {

  BVHNode *current = selectionHistory.top();
  switch (key) {
  case ']':
      ns_aa *=2;
      set_sample_pattern();
      printf("Samples per pixel changed to %lu\n", ns_aa);
      //tm_key = clamp(tm_key + 0.02f, 0.0f, 1.0f);
      break;
  case '[':
      //tm_key = clamp(tm_key - 0.02f, 0.0f, 1.0f);
      ns_aa /=2;
      if (ns_aa < 1) ns_aa = 1;
      set_sample_pattern();
      printf("Samples per pixel changed to %lu\n", ns_aa);
      break;
  case KEYBOARD_UP:
      if (current != bvh->get_root()) {
          selectionHistory.pop();
      }
      break;
  case KEYBOARD_LEFT:
      if (current->l) {
          selectionHistory.push(current->l);
      }
      break;
  case KEYBOARD_RIGHT:
      if (current->l) {
          selectionHistory.push(current->r);
      }
      break;
  case 'a':
  case 'A':
      show_rays = !show_rays;
  default:
      return;
  }
}

// If ns_aa == 1, sample from the middle of the pixel. Otherwise, decompose your
// number ns_aa into a sum of perfect squares, and do stratified sampling on
// each of the respective grids.
void PathTracer::set_sample_pattern() {
  sample_grids.clear();
  if (ns_aa == 1) return;
  int nSamples = ns_aa;
  while (nSamples > 0) {
    int root = (int) sqrt(nSamples);
    int rsq = root * root;
    while (nSamples >= rsq) {
      sample_grids.push_back(root);
      nSamples -= rsq;
    }
  }
}


// ======================================= trace_ray =======================================
/**
 * the old ray-tracer, Unchanged
 **/
Spectrum PathTracer::trace_ray(const Ray &r, bool includeLe) {

  Intersection isect;

  if (!bvh->intersect(r, &isect)) {

    // log ray miss
    #ifdef ENABLE_RAY_LOGGING
    log_ray_miss(r);
    #endif

    return (envLight && includeLe) ? envLight->sample_dir(r) : Spectrum();
  }

  // log ray hit
  #ifdef ENABLE_RAY_LOGGING
  log_ray_hit(r, isect.t);
  #endif

  Spectrum L_out = includeLe ? isect.bsdf->get_emission() : Spectrum();

  const Vector3D& hit_p = r.o + r.d * isect.t;

  // make a coordinate system for a hit point
  // with N aligned with the Z direction.
  Matrix3x3 o2w;
  make_coord_space(o2w, isect.n);
  Matrix3x3 w2o(o2w.T());

  // w_out points towards the source of the ray (e.g.,
  // toward the camera if this is a primary ray)
  const Vector3D& w_out = (w2o * (r.o - hit_p)).unit();
  if (!isect.bsdf->is_delta()) {
    Vector3D dir_to_light;
    float dist_to_light;
    float pr;

    //
    // estimate direct lighting integral
    //
    for (SceneLight* light : scene->lights) {

      // no need to take multiple samples from a point/directional source
      int num_light_samples = light->is_delta_light() ? 1 : ns_area_light;

      // integrate light over the hemisphere about the normal
      for (int i = 0; i < num_light_samples; i++) {

        // returns a vector 'dir_to_light' that is a direction from
        // point hit_p to the point on the light source.  It also returns
        // the distance from point x to this point on the light source.
        // (pr is the probability of randomly selecting the random
        // sample point on the light source -- more on this in part 2)
        const Spectrum& light_L = light->sample_L(hit_p, &dir_to_light, &dist_to_light, &pr);

        // convert direction into coordinate space of the surface, where
        // the surface normal is [0 0 1]
        const Vector3D& w_in = w2o * dir_to_light;
        if (w_in.z < 0) continue;

        // do shadow ray test
        if (!bvh->intersect(Ray(hit_p + EPS_D * isect.n, dir_to_light,
                                dist_to_light))) {
          // note that computing dot(n,w_in) is simple
          // in surface coordinates since the normal is (0,0,1)
          double cos_theta = w_in.z;

          // evaluate surface bsdf
          const Spectrum& f = isect.bsdf->f(w_out, w_in);

          L_out += (cos_theta / (num_light_samples * pr)) * f * light_L;
        }
      }
    }
  }

  //
  // indirect illumination component, recursively trace/reflection
  // rays
  //
  if (r.depth == 0) return L_out;

  float pdf;
  Vector3D w_in;
  const Spectrum& f = isect.bsdf->sample_f(w_out, &w_in, &pdf);

  // Russian Roulette -
  //
  // Pick a uniform random value p, and terminate path if p < terminate_prob.
  // Otherwise, trace the ray in the direction of w_in

  float reflectance = clamp(0.f, 1.f, f.illum());
  float terminate_prob = 1.f - reflectance;

  if (coin_flip(terminate_prob)) return L_out;

  // compute reflected / refracted ray direction
  const Vector3D& w_in_world = (o2w * w_in).unit();
  Ray rec(hit_p + EPS_D * w_in_world, w_in_world, INF_D, r.depth - 1);

  // compute Monte Carlo estimator by tracing ray (result weighted
  // by (1-terminate_prob) to account for Russian roulette)
  double cos_theta = fabs(w_in.z);
  double denom = pdf * (1 - terminate_prob);
  double scale = denom ? cos_theta / denom : 0;

  return L_out + scale * f * trace_ray(rec, isect.bsdf->is_delta());
}

// ======================================= TODO - pathWeight =======================================
/**
 * get weight for a path
 **/
float pathWeight(int i, int j){
  return 1.f / float(i + j + 1);
}

// ======================================= TODO - trace_ray_bpt =======================================
/**
 * the new BDPT ray-tracer
 **/
Spectrum PathTracer::trace_ray_bpt(const Ray &r, size_t x, size_t y) {
  Spectrum Le;
  for (SceneLight* light : scene->lights) {

  /* Case I: Direct ray from light to eye */
    // Eye path length = 0; Light path length = 0
  Intersection isect;
  if (bvh->intersect(r, &isect))
  {
      Spectrum L_out = isect.bsdf->get_emission(); // Le
      sampleBuffer.update_pixel_add(L_out, x, y);
  }

  Ray lightRay(Vector3D(0, 0, 0), Vector3D(0, 0, 0));
  float lightPdf;
  Le = light->sampleLight(&lightRay, &lightPdf);
  if (lightPdf != 0.0f)
  {
    Le = 1.f / lightPdf * Le;
    render_paths(x, y, r, lightRay, Le);
  }
  }
  return Le;
}

// ======================================= TODO - randomWalk =======================================
/**
 * the random walk: generate Eye path and Light path with sample_f() (sample the BSDF);
 * \param cumulative The total attenuation factor
 * \param vertices Store the path
 * \param eye Eye path (true); Light path (false)
 * \param emission Light spectrum
 **/
void PathTracer::randomWalk(Ray ray, std::vector<Vertice> &vertices, bool eye, Spectrum emission) {
  ray.depth = 0;
  Intersection its;
  Spectrum cumulative(1.0f, 1.0f, 1.0f); // Cumulative attenuation factor
  // if (!eye) cumulative *= emission;

  while(true)
  {

    if (!bvh->intersect(Ray(ray.o, ray.d), &its)){
      break;
    }

    BSDF *bsdf = its.bsdf;
    float bsdfPdf;

    Vertice v;
    const Vector3D& hit_p = ray.o + ray.d * its.t;
    v.p = hit_p;
    v.n = its.n;
    v.bsdf = bsdf;

    // make a coordinate system for a hit point
    // with N aligned with the Z direction.
    Matrix3x3 o2w;
    make_coord_space(o2w, its.n);
    Matrix3x3 w2o(o2w.T());

    if (eye)
    {
      // cout<<"EyeRay! ray.depth: "<<ray.depth<<endl;
      v.wo = (w2o * (-ray.d)).unit();
      cumulative = cumulative * its.bsdf->sample_f(v.wo, &v.wi, &bsdfPdf) + its.bsdf->get_emission();
      ray.d = (o2w * v.wi).unit();
      cumulative *= std::abs(v.wi.z) / bsdfPdf;
    }
    else
    {
      const Ray &r = ray;
      // log_ray_hit(r, its.t);
      v.wi = (w2o * (-ray.d)).unit();
      cumulative = cumulative * its.bsdf->sample_f(v.wi, &v.wo, &bsdfPdf);
      ray.d = (o2w * v.wo).unit();
      cumulative *= std::abs(v.wo.z) / bsdfPdf;
    }
    ray.depth ++;

    v.cumulative = cumulative;
    if (ray.depth > 30 || cumulative.illum() < 1E-7) // Repeat extending the path until a maximun length
      break;

    vertices.push_back(v);
    ray.o = hit_p + ray.d * EPS_D; // Origion of the next light
  }
}

// ======================================= TODO - evalPaths =======================================
/**
 * Get total attenuation along a path in case IV (where Eye path length >= 1 && Light path length >= 1)\
 **/
Spectrum PathTracer::evalPath(
  const std::vector<Vertice> &eyePath,
  const std::vector<Vertice> &lightPath,
  int nEye, int nLight) const {

    const Vertice &ev = eyePath[nEye - 1];
    const Vertice &lv = lightPath[nLight - 1];

    Spectrum L(1.0f, 1.0f, 1.0f);

    // Accumulate attenuation from last reflection
    if (nEye > 1)
      L *= eyePath[nEye - 2].cumulative;
    if (nLight > 1)
      L *= lightPath[nLight - 2].cumulative;

    // Get the Geometric Term
    Vector3D etl = lv.p - ev.p;
    float lengthSquared = etl.norm2();
    etl /= std::sqrt(lengthSquared);

    float geoTerm = fabs(dot(etl, ev.n)) * fabs(dot(-etl, lv.n)) / lengthSquared;

    if (lengthSquared < 0.05)
      return Spectrum();

    // make a coordinate system for a hit point
    // with N aligned with the Z direction.
    Matrix3x3 o2w1;
    make_coord_space(o2w1, ev.n);
    Matrix3x3 w2o1(o2w1.T());

    // convert direction into coordinate space of the surface, where the surface normal is [0 0 1]
    L *= ev.bsdf->f(ev.wi, (w2o1 * etl).unit());

    Matrix3x3 o2w2;
    make_coord_space(o2w2, lv.n);
    Matrix3x3 w2o2(o2w2.T());

    L *= lv.bsdf->f((w2o2 * (-etl)).unit(), lv.wo);

    L *= geoTerm;

    return L;
}


// ======================================= TODO - render_paths =======================================
/**
 * Render paths across light and eye paths in Case II, III, and IV
 **/
void PathTracer::render_paths(size_t x, size_t y, const Ray &eyeRay, const Ray &lightRay, const Spectrum &Le){
  Vector3D wi;
  Vector3D onLight;

  std::vector<Vertice> m_eyePath;
  std::vector<Vertice> m_lightPath;

  randomWalk(eyeRay, m_eyePath, true, Le);
  randomWalk(lightRay, m_lightPath, false, Le);

  /* Case II and IV */
  for (int i=1; i<m_eyePath.size()+1; i++){
    const Vertice &ev = m_eyePath[i-1];

    for (SceneLight* light : scene->lights) {

      /* Case II: Classic Ray Tracing */
       // Eye path length > 0; Light path length = 0

      // shoot a ray to the light based on hit_p, return dir_to_light, &dist_to_light, &pr
      Spectrum localLe = light->sampleLightFromP(ev.p, onLight, wi);

      // make a coordinate system for a hit point
      // with N aligned with the Z direction.
      Matrix3x3 o2w;
      make_coord_space(o2w, ev.n);
      Matrix3x3 w2o(o2w.T());

      // convert direction into coordinate space of the surface, where
        // the surface normal is [0 0 1]
      const Vector3D& localWi = (w2o * wi).unit();
      if (localWi.z < 0) continue;

      // do shadow ray test
      if (!bvh->intersect(Ray(ev.p + EPS_D * ev.n, (onLight - ev.p).unit(),
                              (onLight - ev.p).norm() - EPS_D))) {
        if (i > 1)
          localLe *= m_eyePath[i-2].cumulative;

        // note that computing dot(n,w_in) is simple
        // in surface coordinates since the normal is (0,0,1)
        double cos_theta = localWi.z;

        Spectrum s = localLe * ev.bsdf->f(ev.wo, ev.wi) * cos_theta * pathWeight(i, 0);
        s = (1.0 / double(ns_aa)) * s;

        sampleBuffer.update_pixel_add(s, x, y);
      }

      /* Case IV: Bi-Path */
       // Eye path length > 0; Light path length > 0

      for (int j=1; j<m_lightPath.size()+1; j++){
        const Vertice &lv = m_lightPath[j-1];
        // do shadow ray test
        if (!bvh->intersect(Ray(ev.p + EPS_D * ev.n, (lv.p - ev.p).unit(),
                                (lv.p - ev.p).norm() - EPS_D))) {

          Spectrum s = Le * evalPath(m_eyePath, m_lightPath, i, j) * pathWeight(i, j);
          s = (1.0 / double(ns_aa)) * s;

          sampleBuffer.update_pixel_add(s, x, y);
        }
      }

    }
  }

  /* Case III: LightPath directly to eye */
   // Eye path length = 0; Light path length > 0

  for (int j = 1; j < m_lightPath.size()+1; j++){
    const Vertice &lv = m_lightPath[j-1];

    if (!bvh->intersect(Ray(lv.p + EPS_D * lv.n, (camera->pos - lv.p).unit(),
                                (camera->pos - lv.p).norm() - EPS_D))) {
      Spectrum localLe = Le;
      Vector3D wo = camera->pos - lv.p;
      float lengthSquared = wo.norm2();
      wo /= std::sqrt(lengthSquared);

      if (lengthSquared < 0.05)
        continue;

      Matrix3x3 o2w;
      make_coord_space(o2w, lv.n);
      Matrix3x3 w2o(o2w.T());
      Vector3D localWo = (w2o * wo).unit();

      if (j > 1)
        localLe *= m_lightPath[j-2].cumulative;

      localLe *= lv.bsdf->f(lv.wi, localWo) * std::abs(localWo.z) * (1.f / lengthSquared);

      Spectrum s = localLe * pathWeight(0, j);
      s = (1.0 / double(ns_aa)) * s;

      // Where in the film does this ray shoot to?
      Vector2D pixelPos = camera->get_screen_pos(lv.p);
      size_t screenW = sampleBuffer.w;
      size_t screenH = sampleBuffer.h;
      double x = (pixelPos.x + 0.5) * screenW;
      double y = (pixelPos.y + 0.5) * screenH;

      if (x > 0 && x < screenW && y >0 && y < screenH){
        sampleBuffer.update_pixel_add(s, x, y);
      }
    }
  }
}

// ======================================= TODO - raytrace_pixel =======================================
/**
 * Modified to adapt for BDPT option. Generate eye and light path from an eye (camera) ray, and then render
 **/
Spectrum PathTracer::raytrace_pixel(size_t x, size_t y) {


  size_t screenW = sampleBuffer.w;
  size_t screenH = sampleBuffer.h;

  if (sample_grids.empty()) {
    Ray r = camera->generate_ray((x + 0.5) / screenW,
                                 (y + 0.5) / screenH);
    r.depth = max_ray_depth;

    // #ifdef ENABLE_PATH_TRACING
    if (this->useBDPT == 0)
      return trace_ray(r); // use traditional ray-traycing
    // #else
    else
    {
      Spectrum trace_ray_bpt_spt = trace_ray_bpt(r, x, y); // use bdrt
      return trace_ray_bpt_spt;
    }
    // #endif
  }

  Spectrum s = Spectrum();

  for (int gridSize : sample_grids) {
    double cellSize = 1.0 / gridSize;
    for (int subY = 0; subY < gridSize; subY++) {
      for (int subX = 0; subX < gridSize; subX++) {
        const Vector2D &p = gridSampler->get_sample();
        double dx = (subX + p.x) * cellSize;
        double dy = (subY + p.y) * cellSize;
        Ray r = camera->generate_ray((x + dx) / screenW, (y + dy) / screenH);
        r.depth = max_ray_depth;
        
        // #ifdef ENABLE_PATH_TRACING
        if (this->useBDPT == 0)
          s += trace_ray(r, true); // use traditional ray-traycing
        // #else
        else
          Spectrum trace_ray_bpt_spt = trace_ray_bpt(r, x, y); // use bdrt
        // #endif
      }
    }
  }

  return s * (1.0 / ns_aa);
}

// ======================================= TODO - raytrace_pixel =======================================

void PathTracer::raytrace_tile(int tile_x, int tile_y,
                               int tile_w, int tile_h) 
{

  size_t w = sampleBuffer.w;
  size_t h = sampleBuffer.h;

  size_t tile_start_x = tile_x;
  size_t tile_start_y = tile_y;

  size_t tile_end_x = std::min(tile_start_x + tile_w, w);
  size_t tile_end_y = std::min(tile_start_y + tile_h, h);

  size_t tile_idx_x = tile_x / imageTileSize;
  size_t tile_idx_y = tile_y / imageTileSize;
  size_t num_samples_tile = tile_samples[tile_idx_x + tile_idx_y * num_tiles_w];

  for (size_t y = tile_start_y; y < tile_end_y; y++) {
    if (!continueRaytracing) return;
    for (size_t x = tile_start_x; x < tile_end_x; x++) {
        Spectrum s = raytrace_pixel(x, y);
         // #ifdef ENABLE_PATH_TRACING
        if (this->useBDPT == 0)
          sampleBuffer.update_pixel(s, x, y);
        // #endif
    }
  }

  tile_samples[tile_idx_x + tile_idx_y * num_tiles_w] += 1;

  // #ifdef ENABLE_PATH_TRACING
  if (this->useBDPT == 0)
    sampleBuffer.toColor(frameBuffer, tile_start_x, tile_start_y, tile_end_x, tile_end_y);
  // #else
  else
    sampleBuffer.toColor(frameBuffer, 0, 0, w, h); // need to also render tiles in case (iii) which does not originate in the camera ray (pixel)
  // #endif
}

void PathTracer::worker_thread() {

  Timer timer;
  timer.start();

  WorkItem work;
  while (continueRaytracing && workQueue.try_get_work(&work)) {
    raytrace_tile(work.tile_x, work.tile_y, work.tile_w, work.tile_h);
  }

  workerDoneCount++;
  if (!continueRaytracing && workerDoneCount == numWorkerThreads) {
    timer.stop();
    fprintf(stdout, "Canceled!\n");
    state = READY;
  }

  if (continueRaytracing && workerDoneCount == numWorkerThreads) {
    timer.stop();
    fprintf(stdout, "Done! (%.4fs)\n", timer.duration());
    state = DONE;
  }
}

void PathTracer::increase_area_light_sample_count() {
  ns_area_light *= 2;
  fprintf(stdout, "[PathTracer] Area light sample count increased to %zu!\n", ns_area_light);
}

void PathTracer::decrease_area_light_sample_count() {
  if (ns_area_light > 1) ns_area_light /= 2;
  fprintf(stdout, "[PathTracer] Area light sample count decreased to %zu!\n", ns_area_light);
}

void PathTracer::save_image() {

  if (state != DONE) return;

  time_t rawtime;
  time (&rawtime);

  string filename = "Screen Shot ";
  filename += string(ctime(&rawtime));
  filename.erase(filename.end() - 1);
  filename += string(".png");

  uint32_t* frame = &frameBuffer.data[0];
  size_t w = frameBuffer.w;
  size_t h = frameBuffer.h;
  uint32_t* frame_out = new uint32_t[w * h];
  for(size_t i = 0; i < h; ++i) {
    memcpy(frame_out + i * w, frame + (h - i - 1) * w, 4 * w);
  }

  fprintf(stderr, "[PathTracer] Saving to file: %s... ", filename.c_str());
  lodepng::encode(filename, (unsigned char*) frame_out, w, h);
  fprintf(stderr, "Done!\n");
}

}  // namespace CMU462
