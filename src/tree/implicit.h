#pragma once
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include <vector>

class MetaBalls {
public:
  //float cutoff = 0.f;
  MetaBalls(float a, float b) : a(a), b(b){};
  // LineSegment
  float eval(const glm::vec3 p1, const glm::vec3 l1, const glm::vec3 l2) const;

  inline float get_a() const {return a;}
  inline float get_b() const {return b;}
  inline float get_cutoff() const {return b;}
private:
  float a;
  float b;
  float potential(float distance) const;
};
