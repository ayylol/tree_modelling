#include "rendering/EBO.h"

EBO::EBO(const std::vector<GLuint> &indices)
{
    glGenBuffers(1, &ID);
    update(indices);
}

void EBO::update(const std::vector<GLuint> &indices)
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
}

void EBO::bind() { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID); }

void EBO::unbind() { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }

void EBO::cleanup() { glDeleteBuffers(1, &ID); }
