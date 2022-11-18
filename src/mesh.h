#pragma once
#include <vector>

#include <glad/glad.h>

#include <shader.h>

class Mesh
{
    public:
        Mesh(std::vector<GLfloat> vertices, std::vector<GLuint> indices);

        void draw(const Shader& shader);

        std::vector<GLfloat> vertices;
        std::vector<GLuint> indices;
};
