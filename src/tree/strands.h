#pragma once

#include "rendering/mesh.h"

#include "tree/grid.h"
#include "tree/skeleton.h"

class Strands{
public:
    Strands(const Skeleton& tree, Grid& grid);
    Mesh get_mesh() const;
private:
    std::vector<std::vector<glm::vec3>> paths;
    Grid& grid;
};
