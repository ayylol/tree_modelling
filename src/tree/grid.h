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

#include "rendering/mesh.h"
#include "rendering/VBO.h"

class Grid
{
public:
    Grid(
        glm::ivec3 dimensions, 
        float scale = 1.f, 
        glm::vec3 center = {0.f,0.f,0.f}
        );
    
    void occupy_pos(glm::vec3 pos, unsigned int val);
    void occupy_slot(glm::ivec3 pos, unsigned int val);
    void occupy_line(glm::vec3 start, glm::vec3 end, unsigned int val);
    unsigned int get_in_grid(glm::ivec3 index);
    unsigned int get_in_pos(glm::vec3 pos);

    std::vector<glm::ivec3> get_voxels_line(glm::vec3 start, glm::vec3 end);

    bool is_in_grid(glm::ivec3 grid_cell);
    glm::ivec3 pos_to_grid(glm::vec3 pos);
    glm::vec3 grid_to_pos(glm::ivec3 voxel);

    Mesh get_grid_geom();
    Mesh get_bound_geom();
    Mesh get_occupied_geom();
private:
    std::vector<std::vector<std::vector<unsigned int>>> grid;

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
