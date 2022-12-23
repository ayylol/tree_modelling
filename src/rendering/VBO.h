#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <vector>

struct Vertex
{
    glm::vec3 position;
    glm::vec3 color;
    glm::vec3 normal = glm::vec3(0,0,0);
};
template <typename T> class VBO
{
    public:
        GLuint ID;

        VBO(const std::vector<T>& vertices)
        {
            glGenBuffers(1, &ID);
            update(vertices);
        }

        void update(const std::vector<T>& vertices)
        {
            glBindBuffer(GL_ARRAY_BUFFER, ID);
            glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(T), vertices.data(), GL_STATIC_DRAW);
        }
        void bind() { glBindBuffer(GL_ARRAY_BUFFER, ID); }
        void unbind() { glBindBuffer(GL_ARRAY_BUFFER,0); }
        void cleanup() { glDeleteBuffers(1, &ID); }
};
