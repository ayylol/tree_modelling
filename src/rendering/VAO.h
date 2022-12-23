#pragma once

#include <glad/glad.h>
#include "rendering/VBO.h"

template <typename T> class VAO
{
    public:
        GLuint ID;

        VAO() { glGenVertexArrays(1, &ID); }

        void link_attrib(VBO<T>& VBO, GLuint layout, GLuint num_components, GLenum type, GLsizeiptr stride, void* offset)
        {
            VBO.bind();
            glVertexAttribPointer(layout, num_components, type, GL_FALSE, stride, offset);
            glEnableVertexAttribArray(layout);
            VBO.unbind();
        }

        void bind() { glBindVertexArray(ID); }

        void unbind() { glBindVertexArray(0); }

        void cleanup() { glDeleteVertexArrays(1, &ID); }
};
