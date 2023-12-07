#pragma once 

#include <vector>
#include <array>
#include <tuple>
#include <limits>
#include <algorithm>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/hash.hpp"


#include "util/color.h"

#include "rendering/mesh.h"
#include "rendering/VBO.h"
#include "tree/skeleton.h"
#include "tree/implicit.h"

class Grid
{
public:
    Grid(const Skeleton& tree, float percent_overshoot, float scale_factor=1.f);

    float get_scale() { return scale; }
    glm::vec3 get_center() { return center; }
    glm::vec3 get_backbottomleft() { return back_bottom_left; }

    void occupy_pos(glm::vec3 pos, float val);
    void occupy_slot(glm::ivec3 pos, float val);
    void add_slot(glm::ivec3 slot, float val);
    void add_ref(glm::ivec3 slot, size_t segment);
    void add_gradient(glm::ivec3 slot, glm::vec3 val);
    void occupy_line(glm::vec3 start, glm::vec3 end, float val);
    void occupy_path(std::vector<glm::vec3> path, float val);

    // Implicit Filling
    void fill_path(std::vector<glm::vec3> path, Implicit& implicit);
    void fill_path(uint32_t strand_id, std::vector<glm::vec3> path, float max_val, float max_b, float shoot_b, float root_b, size_t inflection_point);
    void fill_line(uint32_t strand_id, glm::vec3 p1, glm::vec3 p2, MetaBalls& implicit);
    void fill_point(glm::vec3 p, Implicit& implicit);
    float fill_skeleton(const Skeleton::Node& node, float min_range);

    bool has_refs(glm::ivec3 index) const;
    float eval_pos(glm::vec3 pos) const;
    float lazy_in_check(glm::ivec3 slot, float threshold);
    float lazy_eval(glm::ivec3 slot);
    glm::vec3 eval_norm(glm::vec3 pos, float step_size=0.0005f) const;
    glm::vec3 eval_gradient(glm::vec3 pos, float step_size=0.0005f) const;

    // TODO: Remove these ////////////////////////
    float get_in_grid(glm::ivec3 index) const;
    float get_in_pos(glm::vec3 pos) const;
    glm::vec3 get_norm_grid(glm::ivec3 index) const;
    glm::vec3 get_norm_pos(glm::vec3 pos) const;
    /////////////////////////////////////////////

    bool line_occluded(glm::vec3 start, glm::vec3 end);

    std::vector<glm::ivec3> get_voxels_line(glm::vec3 start, glm::vec3 end) const;

    bool is_in_grid(glm::ivec3 grid_cell) const;
    glm::ivec3 pos_to_grid(glm::vec3 pos) const;
    glm::vec3 grid_to_pos(glm::ivec3 voxel) const;

    Mesh<VertFlat> get_grid_geom() const;
    Mesh<VertFlat> get_bound_geom() const;
    Mesh<Vertex> get_occupied_voxels(float threshold) const;
    //Mesh<Vertex> get_occupied_geom(float threshold, Grid& texture_space) const;
    Mesh<Vertex> get_occupied_geom(float threshold,Grid& texture_space, std::pair<glm::vec3,glm::vec3>vis_bounds= {glm::vec3(),glm::vec3()});
    Mesh<VertFlat> get_occupied_geom_points(float threshold) const;
    Mesh<VertFlat> get_normals_geom(float threshold) const;

    void export_data(const char * filename);
private:
    struct Eval {
        float val   = 0.0;
        size_t checked = 0;
        std::unordered_map<uint32_t, float> strands_checked;
    };
    std::vector<std::vector<std::vector<std::vector<size_t>>>> grid;
    std::vector<std::vector<std::vector<struct Eval>>> eval_grid;
    std::vector<glm::ivec3> occupied;
    std::vector<struct Segment> segments;

    glm::ivec3 dimensions;
    float scale;
    glm::vec3 center;

    glm::vec3 back_bottom_left;

    // List of face normals for a voxel
    static constexpr std::array<glm::ivec3,6>face_norms = {
        glm::ivec3(1,0,0),  // Right
        glm::ivec3(-1,0,0), // Left
        glm::ivec3(0,1,0),  // Top
        glm::ivec3(0,-1,0), // Bottom
        glm::ivec3(0,0,1),  // Front
        glm::ivec3(0,0,-1)  // Back
    };
  struct Sample{
    glm::vec3 pos;
    float val;
    glm::vec3 norm;
    float col_val;
  };
  using GridCell = std::array<Sample,8>;
  void polygonize(const GridCell &cell, float threshold, std::vector<Vertex> &verts, std::vector<GLuint> &indices) const;
  Vertex vertex_interp(float threshold, const Sample& a, const Sample& b) const; 

};

namespace mc{
  extern const glm::ivec3 cell_order[8];
  extern const int edge_table[256];
  extern const int tri_table[256][16];
};
