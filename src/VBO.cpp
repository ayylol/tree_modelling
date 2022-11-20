#include "VBO.h"

// TODO: Change to be more flexible
VBO::VBO(std::vector<Vertex>& vertices)
{
    glGenBuffers(1, &ID);
    glBindBuffer(GL_ARRAY_BUFFER, ID);
    glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
}

void VBO::bind() { glBindBuffer(GL_ARRAY_BUFFER, ID); }
void VBO::unbind() { glBindBuffer(GL_ARRAY_BUFFER,0); }
void VBO::cleanup() { glDeleteBuffers(1, &ID); }
