#pragma once

#include<string>
#include<fstream>
#include<sstream>
#include<iostream>
#include<cerrno>

#include<glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class Shader 
{
    public:
        GLuint ID;

        Shader(const char* vertexPath, const char* fragmentPath);

        void use() const;
        void cleanup();

        // Uniform functions
        void setUniform(const char* name, glm::mat4 matrix);
        void setUniform(const char* name, glm::vec3 vector);
    private:
        void compile_errors(unsigned int shader, const char* type);
};
