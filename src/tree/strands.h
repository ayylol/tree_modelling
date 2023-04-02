#pragma once

#include <algorithm>
#include <chrono>
#include <random>
#include <utility>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/vector_angle.hpp>

#include "rendering/mesh.h"

#include "tree/grid.h"
#include "tree/implicit.h"
#include "tree/skeleton.h"

#include "util/color.h"
#include "const.h"

glm::vec3 random_color();

class Strands {
public:
  Strands(const Skeleton &tree, Grid &grid);
  Mesh<Vertex> get_mesh() const;
  void add_strand(size_t path_index);
  void add_strands(unsigned int amount);

private:
  Blinn evalfunc;
  std::vector<std::vector<glm::vec3>> paths;
  std::vector<std::vector<glm::vec3>> strands;
  Grid &grid;
};
