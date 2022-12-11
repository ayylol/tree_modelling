#pragma once

#include <vector>

#include <glm/glm.hpp>

glm::vec3 random_color();
glm::vec3 random_brown();
glm::vec3 random_color(const std::vector<glm::vec3>& palette);
