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

float Convolution::eval(glm::vec3 r, glm::vec3 p0, glm::vec3 p1){
    float dist = distance(r,p0,p1);
    if(dist>= cutoff) return 0.f;

    glm::vec3 a = glm::normalize(p1-p0);
    glm::vec3 d = r-p0;

    float l = glm::distance(p0,p1);
    float x = glm::dot(d,a);
    float p = sqrt(1+pow(s,2)*(glm::length2(d)-pow(x,2)));
    float q = sqrt(1+pow(s,2)*(glm::length2(d)+pow(l,2)-2*l*x));
    return x/(2*pow(p,2)*(pow(p,2)+pow(s,2)*pow(x,2)))+
            (l-x)/(2*pow(p,2)*pow(q,2))+
            (std::atan(s*x/p)+std::atan(s*(l-x)/p))/(2*s*pow(p,3));
}
