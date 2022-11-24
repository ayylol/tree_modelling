#include "grid.h"
#include <iostream>

Grid::Grid(
        glm::ivec3 dimensions, 
        float scale, 
        glm::vec3 center
    ):
    dimensions(dimensions),
    scale(scale),
    center(center),
    back_bottom_left(center-glm::vec3(dimensions)*(scale/2.f)),
    grid_geom(std::vector<Vertex>(),std::vector<GLuint>()),
    occupied_geom(std::vector<Vertex>(),std::vector<GLuint>())
{
    gen_grid_geom(); 
}

void Grid::gen_grid_geom()
{
    std::vector<Vertex>& vertices = grid_geom.vertices;
    std::vector<GLuint>& indices = grid_geom.indices;

    // Vertex
    for (int k = 0; k <= dimensions.z; k++){
        for (int j = 0; j <= dimensions.y; j++){
            for (int i = 0; i <= dimensions.x; i++){
                // Vertex Gen
                Vertex point = {back_bottom_left+glm::vec3(i,j,k)*scale,glm::vec3(0.f,0.f,0.f)};
                vertices.push_back(point);
            }
        }
    }
    // Index
    //
    GLuint current = 0;
    const GLuint right = 1;
    const GLuint down = dimensions.x+1;
    const GLuint forward = (dimensions.x+1)*(dimensions.y+1);


    for (int k = 0; k <= dimensions.z; k++){
        for (int j = 0; j < dimensions.y; j++){
            for (int i = 0; i < dimensions.x; i++){
                current = i + j*down + k*forward;
                // Back face
                indices.push_back(current);
                indices.push_back(current+right);

                indices.push_back(current);
                indices.push_back(current+down);

                indices.push_back(current+down+right);
                indices.push_back(current+right);

                indices.push_back(current+down+right);
                indices.push_back(current+down);

                if (k!=dimensions.z){
                    // Connectors to next back face
                    indices.push_back(current);
                    indices.push_back(current+forward);

                    indices.push_back(current+right);
                    indices.push_back(current+right+forward);

                    indices.push_back(current+down);
                    indices.push_back(current+down+forward);

                    indices.push_back(current+down+right);
                    indices.push_back(current+down+right+forward);
                }
            }
        }
    }
    /*
    for(int i = 0; i< (dimensions.x+1)*(dimensions.y+1)*(dimensions.z+1); i++){
        indices.push_back(i);
    }
    */

    grid_geom.update();
}
