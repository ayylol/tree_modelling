#pragma once

#include "glm/fwd.hpp"
#include <vector>
#include "glm/common.hpp"
#include <memory>

class KDTree {
public:
  std::shared_ptr<std::vector<glm::vec2>> points;
  int32_t root;
  struct KDTreeNode {
    int32_t parent=-1, left=-1, right=-1, level=0;
  };
  std::vector<KDTreeNode> tree;
  KDTree(std::shared_ptr<std::vector<glm::vec2>> p);
  KDTree() = default;
  int32_t find_nearest(glm::vec2 p);
private:
  int32_t buildKDTree(std::vector<int32_t> &buffer, int32_t start, int32_t end, int32_t level);
  struct find_nn_t {int node; float d2;};
  find_nn_t find_nearest_(int node, glm::vec2 p, find_nn_t curr_best, glm::vec2 lb, glm::vec2 ub);
};
