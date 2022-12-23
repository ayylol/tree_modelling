#pragma once

#include <glad/glad.h>
#include "rendering/VBO.h"
#include <iostream> //TODO DELETE THIS

template <typename T> class VAO
{
    public:
        GLuint ID;

        VAO() { glGenVertexArrays(1, &ID); }

        void link_attribs(VBO<T>& vbo){
            // Loop through attributes, linking them
            for ( size_t i=0; i<T::elements; i++ ){
                link_attrib(vbo, i, T::info[i].num_components, T::info[i].type,
                        sizeof(T), (void*)(T::info[i].offset)); 
            }
        }

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
