#pragma once
#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "rendering/shader.h"
#include "rendering/camera.h"
#include "rendering/VAO.h"
#include "rendering/VBO.h"
#include "rendering/EBO.h"

class Mesh
{
    public:
        Mesh(std::vector<Vertex> vertices, std::vector<GLuint> indices);

        void update();
        void draw(const Shader& shader, const Camera& cam, GLenum mode);

        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;
    private:
        VAO vao;
};
