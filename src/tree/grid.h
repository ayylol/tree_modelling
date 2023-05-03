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

#include "util/color.h"

#include "rendering/mesh.h"
#include "rendering/VBO.h"
#include "tree/skeleton.h"
#include "tree/implicit.h"

class Grid
{
public:
    Grid(
        glm::ivec3 dimensions, 
        float scale = 1.f, 
        glm::vec3 back_bottom_left = {0.f,0.f,0.f}
        );
    Grid(const Skeleton& tree, float percent_overshoot, float scale_factor=1.f);

    float get_scale() { return scale; }
    glm::vec3 get_center() { return center; }
    glm::vec3 get_backbottomleft() { return back_bottom_left; }

    void occupy_pos(glm::vec3 pos, float val);
    void occupy_slot(glm::ivec3 pos, float val);
    void add_slot(glm::ivec3 slot, float val);
    void occupy_line(glm::vec3 start, glm::vec3 end, float val);
    void occupy_path(std::vector<glm::vec3> path, float val);

    // Implicit Filling
    void fill_path(std::vector<glm::vec3> path, Implicit& implicit, float offset);
    void fill_line(glm::vec3 p1, glm::vec3 p2, Implicit& implicit);
    void fill_point(glm::vec3 p, Implicit& implicit);

    float get_in_grid(glm::ivec3 index) const;
    float get_in_pos(glm::vec3 pos) const;
    glm::vec3 get_norm_grid(glm::ivec3 index) const;
    glm::vec3 get_norm_pos(glm::vec3 pos) const;

    bool line_occluded(glm::vec3 start, glm::vec3 end);

    std::vector<glm::ivec3> get_voxels_line(glm::vec3 start, glm::vec3 end) const;

    bool is_in_grid(glm::ivec3 grid_cell) const;
    glm::ivec3 pos_to_grid(glm::vec3 pos) const;
    glm::vec3 grid_to_pos(glm::ivec3 voxel) const;

    Mesh<VertFlat> get_grid_geom() const;
    Mesh<VertFlat> get_bound_geom() const;
    Mesh<Vertex> get_occupied_geom(float threshold) const;
    Mesh<VertFlat> get_occupied_geom_points(float threshold) const;
    Mesh<VertFlat> get_normals_geom(float threshold) const;

    void smooth_grid();

    void export_data(const char * filename);
private:
    std::vector<std::vector<std::vector<float>>> grid;
    std::vector<glm::ivec3> occupied;

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

};
