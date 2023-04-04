#pragma once
#include <glm/glm.hpp>

#include <vector>
class Implicit {
public:
  float cutoff = 0.f;
  // Point
  virtual float eval(glm::vec3 p1, glm::vec3 p2) = 0;
  // LineSegment
  virtual float eval(glm::vec3 p1, glm::vec3 a, glm::vec3 b) = 0;
  // Path
  virtual float eval(glm::vec3 position, const std::vector<glm::vec3> &strand,
                     std::size_t hint = 0) = 0;
};

class DistanceField : public Implicit {
public:
  DistanceField(float cutoff) { this->cutoff = cutoff; };
  // Point
  float eval(glm::vec3 p1, glm::vec3 p2);
  // LineSegment
  float eval(glm::vec3 p1, glm::vec3 a, glm::vec3 b);
  // Path
  float eval(glm::vec3 position, const std::vector<glm::vec3> &strand,
             std::size_t hint = 0);

private:
  virtual float potential(float distance) = 0;
};

class MetaBalls : public DistanceField {
public:
  MetaBalls(float cutoff) : DistanceField(cutoff){};
private:
  float potential(float distance);
};

class Blinn : public DistanceField {
public:
  Blinn(float radius, float blobiness, float cutoff_val);
private:
  float radius;
  float blobiness;
  float potential(float distance);
};