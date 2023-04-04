#pragma once

#include <glm/glm.hpp>
#include <vector>

// NOTE: IMPLEMENTED IN STRANDS FILE
// TODO put in a namespace
std::pair<std::size_t, glm::vec3>
closest_node_on_path(glm::vec3 point, const std::vector<glm::vec3> &path,
                     int start_index, int overshoot);

glm::vec3 closest_on_path(glm::vec3 point, const std::vector<glm::vec3> &path,
                          int start_index, int overshoot);

glm::vec3 closest_on_line(glm::vec3 p, glm::vec3 a, glm::vec3 b);

float distance(glm::vec3 p1, glm::vec3 p2);
float distance(glm::vec3 p1, glm::vec3 a, glm::vec3 b);
float distance(glm::vec3 p, const std::vector<glm::vec3> &strand,
               std::size_t hint = 0);
