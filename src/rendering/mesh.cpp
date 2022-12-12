#include "rendering/mesh.h"
#include <iostream>

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<GLuint> indices):
    vertices(vertices),
    indices(indices)
{
    this->update();
}

void Mesh::update(){
    vao.bind();

    VBO vbo(vertices);
    EBO ebo(indices);

    vao.link_attrib(vbo, 0, 3, GL_FLOAT, 9 * sizeof(float), (void*)0);
    vao.link_attrib(vbo, 1, 3, GL_FLOAT, 9 * sizeof(float), (void*)(3*sizeof(float)));
    vao.link_attrib(vbo, 2, 3, GL_FLOAT, 9 * sizeof(float), (void*)(6*sizeof(float)));

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
    GLuint camPosLoc = glGetUniformLocation(shader.ID, "camPos");
    glm::vec3 camPos = camera.get_position();
    glUniform3f(camPosLoc, camPos.x, camPos.y, camPos.z);
    //std::cout <<camPosLoc<<std::endl;

    vao.bind();
    glDrawElements(mode, indices.size(), GL_UNSIGNED_INT, 0);
}
