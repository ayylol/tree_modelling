#include "rendering/VBO.h"

VBO::VBO(const std::vector<Vertex>& vertices)
{
    glGenBuffers(1, &ID);
    update(vertices);
}

void VBO::update(const std::vector<Vertex>& vertices)
{
    glBindBuffer(GL_ARRAY_BUFFER, ID);
    glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
}
void VBO::bind() { glBindBuffer(GL_ARRAY_BUFFER, ID); }
void VBO::unbind() { glBindBuffer(GL_ARRAY_BUFFER,0); }
void VBO::cleanup() { glDeleteBuffers(1, &ID); }
