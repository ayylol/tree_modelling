#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <vector>

// TODO: add normal for phong
struct Vertex
{
    glm::vec3 position;
    glm::vec3 color;
};
class VBO
{
    public:
        GLuint ID;

        VBO(std::vector<Vertex>& vertices);

        void bind();
        void unbind();
        void cleanup();
};
