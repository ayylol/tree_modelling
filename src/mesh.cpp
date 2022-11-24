#include "mesh.h"
#include <iostream>

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<GLuint> indices)
{
    this->vertices = vertices;
    this->indices = indices;
    this->update();
}

void Mesh::update(){
    vao.bind();

    VBO vbo(vertices);
    EBO ebo(indices);

    vao.link_attrib(vbo, 0, 3, GL_FLOAT, 6 * sizeof(float), (void*)0);
    vao.link_attrib(vbo, 1, 3, GL_FLOAT, 6 * sizeof(float), (void*)(3*sizeof(float)));

    vao.unbind();
    vbo.unbind();
    ebo.unbind();
}

void Mesh::draw(const Shader& shader, const Camera& camera, GLenum mode)
{
    shader.use();
    glm::mat4 cam_mat = camera.get_matrix();
    GLuint camLoc = glGetUniformLocation(shader.ID, "cam");
    glUniformMatrix4fv(camLoc, 1, GL_FALSE, glm::value_ptr(cam_mat));

    vao.bind();
    glDrawElements(mode, indices.size(), GL_UNSIGNED_INT, 0);
}
