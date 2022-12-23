#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <array>
#include <vector>

struct VertexInfo{
    size_t offset; 
    size_t num_components;
    GLenum type;
};

struct Vertex
{
    // Data
    glm::vec3 position;
    glm::vec3 color;
    glm::vec3 normal = glm::vec3(0,0,0);

    // Vertex info
    static constexpr size_t elements = 3;
    static constexpr std::array<VertexInfo, elements> info = {
        VertexInfo{0,3,GL_FLOAT},
        VertexInfo{3*sizeof(float),3,GL_FLOAT},
        VertexInfo{6*sizeof(float),3,GL_FLOAT}
    };
};

struct VertFlat
{
    // Data
    glm::vec3 position;
    glm::vec3 color;

    // Vertex info
    static constexpr size_t elements = 2;
    static constexpr std::array<VertexInfo, elements> info = {
        VertexInfo{0,3,GL_FLOAT},
        VertexInfo{3*sizeof(float),3,GL_FLOAT}
    };
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
