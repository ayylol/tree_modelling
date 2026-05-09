#include "kdtree.h"
#include "glm/gtx/norm.hpp"
#include "glm/gtx/io.hpp"
#include <algorithm>
#include <numeric>
#include <iostream>
#include <cfloat>

KDTree::KDTree(std::shared_ptr<std::vector<glm::vec2>> p) : points(p) {
  std::vector<int32_t>buff(points->size());
  tree = std::vector<KDTreeNode>(points->size());
  std::iota(buff.begin(), buff.end(), 0);
  root = buildKDTree(buff, 0, buff.size(), true);
  for (int i=0;i<tree.size();i++){ 
    assert(i==root||tree[i].parent!=-1); 
    assert(tree[i].left==-1||tree[tree[i].left].parent==i);
    assert(tree[i].right==-1||tree[tree[i].right].parent==i);
  }
}
int32_t KDTree::buildKDTree(std::vector<int32_t> &buffer, int32_t start, int32_t end, int32_t level){
  if (start>=end) return -1;
  auto cmp = [&](int32_t a ,int32_t b){
    return level%2==0 ? 
      (*points)[a].x < (*points)[b].x:
      (*points)[a].y < (*points)[b].y;
  };
  std::sort(buffer.begin()+start, buffer.begin()+end, cmp);
  int32_t buff_idx = start+(end-start)/2;
  int32_t node_idx = buffer[buff_idx];

  tree[node_idx].level=level;
  tree[node_idx].left=buildKDTree(buffer, start, buff_idx, level+1);
  tree[node_idx].right=buildKDTree(buffer, buff_idx+1, end, level+1);

  if(tree[node_idx].left!=-1)tree[tree[node_idx].left].parent=node_idx;
  if(tree[node_idx].right!=-1)tree[tree[node_idx].right].parent=node_idx;

  return node_idx;
}
int32_t KDTree::find_nearest(glm::vec2 p){
  return find_nearest_(root, p, (find_nn_t){.node=root, .d2=FLT_MAX}, 
      glm::vec2(-FLT_MAX, -FLT_MAX), glm::vec2(FLT_MAX, FLT_MAX)).node;
}
float aabb_dist2(glm::vec2 p, glm::vec2 lb, glm::vec2 ub){
  glm::vec2 h_aabb=(ub-lb)/2.f;
  glm::vec2 c_aabb=lb+h_aabb;
  glm::vec2 dp = glm::abs(p-c_aabb)-glm::abs(h_aabb);
  dp.x = dp.x<0 ? 0 : dp.x;
  dp.y = dp.y<0 ? 0 : dp.y;
  return glm::length2(dp);
}
KDTree::find_nn_t KDTree::find_nearest_(int node, glm::vec2 p, find_nn_t curr_best, glm::vec2 lb, glm::vec2 ub){
  if (node==-1) return curr_best;
  // PRUNE
  float d_bound = aabb_dist2(p, lb, ub);
  if (d_bound>curr_best.d2) {
    return curr_best;
  }

  // check this node
  find_nn_t this_res = {.node=node, .d2=glm::length2(p-(*points)[node])};
  if (curr_best.d2>this_res.d2){ curr_best=this_res; }

  // Get new boundaries
  glm::vec2 new_lb=lb;
  glm::vec2 new_ub=ub;
  if (tree[node].level%2==0){
    new_lb.x=(*points)[node].x;
    new_ub.x=(*points)[node].x;
  }else{
    new_lb.y=(*points)[node].y;
    new_ub.y=(*points)[node].y;
  }
  // Explore Tree
  find_nn_t left_best = find_nearest_(tree[node].left, p, curr_best, lb, new_ub);
  if (curr_best.d2>left_best.d2){ curr_best=left_best; }
  find_nn_t right_best = find_nearest_(tree[node].right, p, curr_best, new_lb, ub);
  if (curr_best.d2>right_best.d2){ curr_best=right_best; }

  return curr_best;
}
