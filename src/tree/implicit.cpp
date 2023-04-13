#include "tree/implicit.h"
#include "glm/geometric.hpp"
#include "util/geometry.h"
#include <cmath>
#include <glm/gtc/constants.hpp>
#include <iostream>

using std::pow;
using std::sqrt;
using std::log;
using std::exp;

float DistanceField::eval(glm::vec3 p1, glm::vec3 p2) {
  float d = distance(p1, p2);
  if (d >= cutoff)
    return 0.f;
  return potential(d);
}
float DistanceField::eval(glm::vec3 p1, glm::vec3 a, glm::vec3 b) {
  float d = distance(p1, a, b);
  if (d >= cutoff)
    return 0.f;
  return potential(d);
}

float DistanceField::eval(glm::vec3 p1, const std::vector<glm::vec3> &strand,
                          std::size_t hint) {
  float d = distance(p1, strand, hint);
  if (d >= cutoff)
    return 0.f;
  return potential(d);
}

float MetaBalls::potential(float distance) {
  if (distance <= b / 3) {
    return a * (1 - 3 * pow(distance / b, 2));
  }
  return (a * 3 / 2) * pow((1 - distance / b), 2);
}

Blinn::Blinn(float radius, float blobiness, float cutoff_val)
    : radius(radius), blobiness(blobiness),
      DistanceField(radius * sqrt(log(cutoff_val) / blobiness + 1)) {}

float Blinn::potential(float distance) {
  return exp(blobiness * (pow(distance, 2) / pow(radius, 2) - 1));
}
