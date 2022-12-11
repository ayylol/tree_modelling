#pragma once

#include <vector>
#include <random>
#include <chrono>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/vector_angle.hpp>

#include "rendering/mesh.h"

#include "tree/grid.h"
#include "tree/skeleton.h"

#include "util/color.h"

glm::vec3 random_color();

class Strands{
public:
    Strands(const Skeleton& tree, Grid& grid);
    Mesh get_mesh() const;
    void add_strand(size_t path_index);
    void add_strands(unsigned int amount);
private:
    std::vector<std::vector<glm::vec3>> paths;
    std::vector<std::vector<glm::vec3>> strands;
    Grid& grid;
};
