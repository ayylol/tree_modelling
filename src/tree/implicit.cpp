#include "tree/implicit.h"
#include <cmath>
#include <glm/geometric.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/constants.hpp>
#include <iostream>
#include "util/geometry.h"
#include "glm/gtx/fast_square_root.hpp"

using std::pow;
using std::sqrt;
using std::log;
using std::exp;

float MetaBalls::eval(const glm::vec3 p1, const glm::vec3 l1, const glm::vec3 l2) const{
  float d = distance2(p1, l1, l2);
  if (d >= b*b)
    return 0.f;
  return potential(glm::fastSqrt(d));
}

float MetaBalls::potential(float distance) const {
  if (distance <= b / 3) {
    return a * (1 - 3 * pow(distance / b, 2));
  }
  return (a * 3 / 2) * pow((1 - distance / b), 2);
}
