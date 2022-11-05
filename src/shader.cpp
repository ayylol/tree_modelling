#include "shader.h"

std::string get_file_contents(const char* filename)
{
  std::ifstream in(filename, std::ios::binary);
  if (in)
  {
    std::string contents;
    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    in.close();
    return(contents);
  }
  throw(errno);
}

void Shader::use() { glUseProgram(ID); }
void Shader::cleanup() { glDeleteProgram(ID); }

Shader::Shader(const char* vertexPath, const char* fragmentPath)
{
  // Get the strings for the shader code
  std::string vertex_code = get_file_contents(vertexPath);
  std::string fragment_code = get_file_contents(fragmentPath);

  // Convert shader code into c strings
  const char* vertex_source = vertex_code.c_str();
  const char* fragment_source = fragment_code.c_str();

  // Create the vertex shader
  GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vertex_source, NULL);
  glCompileShader(vertex_shader);

  GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fragment_source, NULL);
  glCompileShader(fragment_shader);

  ID = glCreateProgram();
  glAttachShader(ID, vertex_shader);
  glAttachShader(ID, fragment_shader);
  glLinkProgram(ID);

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);
}
