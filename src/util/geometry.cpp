#include "util/geometry.h"
#include <algorithm>
#include <glm/gtx/norm.hpp>
#include <iostream>
#include <math.h>
#include <utility>

#define ERR 0.0001f
std::pair<float, float> quadratic_solver(float a, float b, float c){
    double disc = b*b-4.f*a*c;
    std::cout<<disc<<std::endl;
    if (disc < -ERR) return std::make_pair(NAN, NAN);
    if (disc >= -ERR && disc <= ERR ) disc = 0.f;
    float r1 = (-b-sqrt(disc))/2.f*a;
    float r2 = (-b+sqrt(disc))/2.f*a;
    return std::make_pair(std::min(r1,r2),std::max(r1,r2));
}

// TODO DELETE THIS
std::pair<size_t, glm::vec3>
closest_node_on_path(glm::vec3 point, const std::vector<glm::mat4> &path,
                     int start_index, int overshoot) {
  // Initialize vars for hill-climb
  int current_closest_index = start_index;
  glm::vec3 current_closest_point = frame_position(path[start_index]);
  float lowest_dist2 = glm::distance2(point, current_closest_point);

  int point_checking = 1;
  int overshot = -1;

  // Look for closest point on path
  while (overshoot >= overshot) {
    float last_lowest_dist2 = lowest_dist2;
    if (start_index + point_checking <= path.size() - 1 &&
        glm::distance2(point, frame_position(path[start_index + point_checking])) <
            lowest_dist2) {
      lowest_dist2 = glm::distance2(point, frame_position(path[start_index + point_checking]));
      current_closest_index = start_index + point_checking;
    }
    if (start_index - point_checking >= 0 &&
        glm::distance2(point, frame_position(path[start_index - point_checking])) <
            lowest_dist2) {
      lowest_dist2 = glm::distance2(point, frame_position(path[start_index - point_checking]));
      current_closest_index = start_index - point_checking;
    }

    // Increase overshot counter or reset it
    if (lowest_dist2 == last_lowest_dist2) {
      overshot++;
    } else {
      overshot = -1;
    }
    point_checking++;
  }
  current_closest_point = frame_position(path[current_closest_index]);
  return std::make_pair(current_closest_index, current_closest_point);
}

glm::vec3 closest_on_path(glm::vec3 point, const std::vector<glm::vec3> &path,
                          int start_index, int overshoot) {
  int i = start_index;
  int overshot = -1;
  float lowest_dist2 = FLT_MAX;
  glm::vec3 closest_point = path[start_index];
  do {
    glm::vec3 next_closest = closest_on_line(point, path[i], path[i + 1]);
    float dist2 = glm::distance2(next_closest, point);
    if (dist2 < lowest_dist2) {
      closest_point = next_closest;
      lowest_dist2 = dist2;
      overshot = -1;
    } else {
      overshot++;
    }
    i++;
  } while (overshoot >= overshot && i < path.size() - 1);
  return closest_point;
}

glm::vec3 closest_on_line(glm::vec3 p, glm::vec3 a, glm::vec3 b) {
  glm::vec3 ab = b - a;
  glm::vec3 ap = p - a;
  float proj = glm::dot(ab, ap);
  float abLen2 = glm::length2(ab);
  return std::clamp((proj / abLen2), 0.f, 1.f) * ab + a;
}

float distance(glm::vec3 p1, glm::vec3 p2) { return glm::distance(p1, p2); }
float distance(glm::vec3 p1, glm::vec3 a, glm::vec3 b) {
  return glm::distance(p1, closest_on_line(p1, a, b));
};

float distance(glm::vec3 position, const std::vector<glm::vec3> &strand,
               std::size_t hint) {
  glm::vec3 closest = closest_on_path(position, strand, hint, 3);
  return glm::distance(position, closest);
}
