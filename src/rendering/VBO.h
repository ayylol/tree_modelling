#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <array>
#include <vector>

struct Vertex
{
    // Data
    glm::vec3 position;
    glm::vec3 color;
    glm::vec3 normal = glm::vec3(0,0,0);

    // Vertex info
    static constexpr size_t elements = 3;
    static constexpr std::array<size_t, elements> offsets = {0,3*sizeof(float),6*sizeof(float)};
    static constexpr std::array<GLenum, elements> types = {GL_FLOAT, GL_FLOAT, GL_FLOAT};
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
