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

template <typename T=Vertex> class Mesh
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

        void draw(const Shader& shader, const Camera& camera, GLenum mode)
        {
            shader.use();
            glm::mat4 cam_mat = camera.get_matrix();
            GLuint camLoc = glGetUniformLocation(shader.ID, "cam");
            glUniformMatrix4fv(camLoc, 1, GL_FALSE, glm::value_ptr(cam_mat));
            GLuint camPosLoc = glGetUniformLocation(shader.ID, "camPos");
            glm::vec3 camPos = camera.get_position();
            glUniform3f(camPosLoc, camPos.x, camPos.y, camPos.z);

            vao.bind();
            glDrawElements(mode, indices.size(), GL_UNSIGNED_INT, 0);
        }
    private:
        VAO<T> vao;
};
