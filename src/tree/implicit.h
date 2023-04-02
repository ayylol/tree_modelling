#pragma once
#include <glm/glm.hpp>

#include <vector>
class Implicit {
public:
  float cutoff = 0.f;
  virtual float eval(glm::vec3 position, const std::vector<glm::vec3> &strand,
                     std::size_t hint = 0) = 0;
};

class DistanceField : public Implicit {
public:
  DistanceField(float cutoff) {this->cutoff=cutoff;};
  float eval(glm::vec3 position, const std::vector<glm::vec3> &strand,
                     std::size_t hint = 0);

private:
  virtual float potential(float distance) = 0;
  float distance(glm::vec3 position, const std::vector<glm::vec3> &strand,
                 std::size_t hint = 0);
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
