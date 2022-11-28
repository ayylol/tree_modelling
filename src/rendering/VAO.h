#pragma once

#include <glad/glad.h>
#include "rendering/VBO.h"

class VAO
{
    public:
        GLuint ID;

        VAO();

        void link_attrib(VBO& VBO, GLuint layout, GLuint num_components, GLenum type, GLsizeiptr stride, void* offset);
        void bind();
        void unbind();
        void cleanup();
};
