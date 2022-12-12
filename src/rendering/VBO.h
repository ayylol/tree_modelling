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
class VBO
{
    public:
        GLuint ID;

        VBO(const std::vector<Vertex>& vertices);

        void update(const std::vector<Vertex>& vertices);
        void bind();
        void unbind();
        void cleanup();
};
