#pragma once

#include "rendering/mesh.h"

#include "tree/grid.h"
#include "tree/skeleton.h"

#include "util/color.h"

glm::vec3 random_color();

class Strands{
public:
    Strands(const Skeleton& tree, Grid& grid);
    Mesh get_mesh() const;
private:
    std::vector<std::vector<glm::vec3>> paths;
    Grid& grid;
};
