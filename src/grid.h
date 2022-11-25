#pragma once 

#include <vector>
#include <algorithm>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "mesh.h"
#include "VBO.h"

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

    void gen_grid_geom();
    void gen_occupied_geom();

    //TODO: make private with draw method
    Mesh grid_geom;
    Mesh occupied_geom;
private:
    std::vector<std::vector<std::vector<unsigned int>>> grid;

    glm::ivec3 dimensions;
    float scale;
    glm::vec3 center;

    glm::vec3 back_bottom_left;
};
