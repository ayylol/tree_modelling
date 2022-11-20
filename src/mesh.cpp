#include "mesh.h"
#include <iostream>

Mesh::Mesh(std::vector<GLfloat> vertices, std::vector<GLuint> indices)
{
    this->vertices = vertices;
    this->indices = indices;
    vao.bind();

    VBO vbo(&vertices[0], vertices.size()*sizeof(float));
    EBO ebo(&indices[0], indices.size()*sizeof(unsigned int));

    vao.link_attrib(vbo, 0, 3, GL_FLOAT, 6 * sizeof(float), (void*)0);
    vao.link_attrib(vbo, 1, 3, GL_FLOAT, 6 * sizeof(float), (void*)(3*sizeof(float)));

    vao.unbind();
    vbo.unbind();
    ebo.unbind();
}

void Mesh::draw(const Shader& shader, const Camera& camera)
{
    shader.use();
    glm::mat4 cam_mat = camera.get_matrix();
    GLuint camLoc = glGetUniformLocation(shader.ID, "cam");
    glUniformMatrix4fv(camLoc, 1, GL_FALSE, glm::value_ptr(cam_mat));

    vao.bind();
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
}
