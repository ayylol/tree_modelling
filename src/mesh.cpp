#include "mesh.h"
#include <iostream>

Mesh::Mesh(std::vector<GLfloat> verts, std::vector<GLuint> indices)
{
    this->vertices = vertices;
    this->indices = indices;
}

void Mesh::draw(const Shader& shader)
{
    shader.use();
    std::cout<<"trying to draw"<<std::endl;
}
