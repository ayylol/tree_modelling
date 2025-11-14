#include "tree/implicit.h"
#include <cmath>
#include <glm/geometric.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/constants.hpp>
#include <iostream>
#include "util/geometry.h"

using std::pow;
using std::sqrt;
using std::log;
using std::exp;

float DistanceField::eval(glm::vec3 p1, glm::vec3 p2) const {
  float d = distance(p1, p2);
  if (d >= cutoff)
    return 0.f;
  return potential(d);
}

float DistanceField::eval(glm::vec3 p1, glm::vec3 a, glm::vec3 b) const {
  float d = distance(p1, a, b);
  if (d >= cutoff)
    return 0.f;
  return potential(d);
}

float DistanceField::eval(glm::vec3 p1, const std::vector<glm::vec3> &strand,
                          std::size_t hint) const {
  float d = distance(p1, strand, hint);
  if (d >= cutoff)
    return 0.f;
  return potential(d);
}

float MetaBalls::potential(float distance) const {
  if (distance <= b / 3) {
    return a * (1 - 3 * pow(distance / b, 2));
  }
  return (a * 3 / 2) * pow((1 - distance / b), 2);
}
