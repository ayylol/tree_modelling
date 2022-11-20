#pragma once
#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <shader.h>
#include <camera.h>
#include <VAO.h>
#include <VBO.h>
#include <EBO.h>

class Mesh
{
    public:
        Mesh(std::vector<Vertex> vertices, std::vector<GLuint> indices);

        void draw(const Shader& shader, const Camera& cam);

        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;
    private:
        VAO vao;
};
