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

template <typename T> class Mesh
{
    public:
        std::vector<T> vertices;
        std::vector<GLuint> indices;

        Mesh(std::vector<T> vertices, std::vector<GLuint> indices):
            vertices(vertices),
            indices(indices)
        {
            this->update();
        }

        // TODO: see if necessary
        void update(){
            vao.bind();

            VBO<T> vbo(vertices);
            EBO ebo(indices);

            vao.link_attribs(vbo);

            vao.unbind();
            vbo.unbind();
            ebo.unbind();
        }

        void draw(Shader& shader, const Camera& camera, GLenum mode)
        {
            shader.use();

            shader.setUniform("cam", camera.get_matrix());
            shader.setUniform("camPos", camera.get_position());

            vao.bind();
            glDrawElements(mode, indices.size(), GL_UNSIGNED_INT, 0);
        }
    private:
        VAO<T> vao;
};
