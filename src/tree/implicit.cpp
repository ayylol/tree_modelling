#include "tree/implicit.h"
#include "glm/geometric.hpp"
#include <glm/gtc/constants.hpp>
#include <cmath>
#include <iostream>
#include "util/geometry.h"

float DistanceField::eval(glm::vec3 position, const std::vector<glm::vec3> &strand,
           std::size_t hint) {
    float d = distance(position,strand,hint);
    if (d>=cutoff) return 0.f;
    return potential(d);
}

// TODO: Should be general
float DistanceField::distance(glm::vec3 position, const std::vector<glm::vec3> &strand,
           std::size_t hint) {
    glm::vec3 closest = closest_on_path(position, strand, hint, 3).second;
    return glm::distance(position,closest);
}

float MetaBalls::potential(float distance){
    if (distance <= cutoff/3){
        return 1-3*std::pow(distance/cutoff,2);
    }
    return (3.f/2)*std::pow((1-distance/cutoff),2);
}
