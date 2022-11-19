#pragma once

#include<string>
#include<fstream>
#include<sstream>
#include<iostream>
#include<cerrno>

#include<glad/glad.h>
#include <GLFW/glfw3.h>

std::string get_file_contents(const char* filename);

class Shader 
{
    public:
        GLuint ID;

        Shader(const char* vertexPath, const char* fragmentPath);

        void use() const;
        void cleanup();
    private:
        void compile_errors(unsigned int shader, const char* type);
};
