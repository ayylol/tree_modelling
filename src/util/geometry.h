#pragma once

#include <glm/glm.hpp>
#include <vector>

// NOTE: IMPLEMENTED IN STRANDS FILE
std::pair<std::size_t, glm::vec3> closest_on_path(glm::vec3 point,
                                             const std::vector<glm::vec3> &path,
                                             int start_index, int overshoot);
