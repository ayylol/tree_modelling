#pragma once 

#include <vector>

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

    std::vector<std::vector<std::vector<unsigned int>>> grid;

    Mesh grid_geom;
    Mesh occupied_geom;
private:
    glm::ivec3 dimensions;
    float scale;
    glm::vec3 center;

    glm::vec3 back_bottom_left;
    void gen_grid_geom();
};
