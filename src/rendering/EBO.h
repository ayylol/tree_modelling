#pragma once

#include <vector>

#include <glad/glad.h>

class EBO
{
    public:
        GLuint ID;

        EBO(const std::vector<GLuint> &indices);

        void update(const std::vector<GLuint> &indices);
        void bind();
        void unbind();
        void cleanup();
};
