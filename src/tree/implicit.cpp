#include "tree/implicit.h"
#include "glm/geometric.hpp"
#include "util/geometry.h"
#include <cmath>
#include <glm/gtc/constants.hpp>
#include <iostream>

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

float DistanceField::eval(glm::vec3 p1, const std::vector<glm::vec3> &strand, std::size_t hint) {
  float d = distance(p1, strand, hint);
  if (d >= cutoff)
    return 0.f;
  return potential(d);
}

float MetaBalls::potential(float distance) {
  if (distance <= cutoff / 3) {
    return 1 - 3 * std::pow(distance / cutoff, 2);
  }
  return (3.f / 2) * std::pow((1 - distance / cutoff), 2);
}

Blinn::Blinn(float radius, float blobiness, float cutoff_val)
    : radius(radius), blobiness(blobiness),
      DistanceField(radius * std::sqrt(std::log(cutoff_val) / blobiness + 1)) {}

float Blinn::potential(float distance) {
  return std::exp(blobiness *
                  (std::pow(distance, 2) / std::pow(radius, 2) - 1));
}
