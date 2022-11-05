#include <iostream>
#include <cmath>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "shader.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

int main(void) 
{
  //GLFW init
  if(!glfwInit())
  {
    std::cout << "Failed to initialize GLFW" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // Window Creation
  GLFWwindow* window = glfwCreateWindow(SCR_WIDTH,SCR_HEIGHT, "tree strands (DEBUG)", NULL, NULL);
  if (window == NULL)
  {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  //GLAD init
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
  {
    std::cout << "Failed to initialize GLAD" << std::endl;
    glfwTerminate();
    return -1;
  }

  // TODO: remove this later, just for test
  // Vertices coordinates
	GLfloat verts[] =
	{
		-0.5f, -0.5f * float(sqrt(3)) / 3, 0.0f, // Lower left corner
		0.5f, -0.5f * float(sqrt(3)) / 3, 0.0f, // Lower right corner
		0.0f, 0.5f * float(sqrt(3)) * 2 / 3, 0.0f, // Upper corner
		-0.5f / 2, 0.5f * float(sqrt(3)) / 6, 0.0f, // Inner left
		0.5f / 2, 0.5f * float(sqrt(3)) / 6, 0.0f, // Inner right
		0.0f, -0.5f * float(sqrt(3)) / 3, 0.0f // Inner down
	};

	// Indices for vertices order
	GLuint indices[] =
	{
		0, 3, 5, // Lower left triangle
		3, 2, 4, // Lower right triangle
		5, 4, 1 // Upper triangle
	};

  // readying viewport
  glViewport(0,0,SCR_WIDTH,SCR_HEIGHT);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  // Readying Shaders 
  Shader default_shader("resources/shaders/default.vert", "resources/shaders/default.frag");

  VAO VAO1;
  VAO1.bind();

  VBO VBO1(verts, sizeof(verts));
  EBO EBO1(indices, sizeof(indices));

  VAO1.linkVBO(VBO1, 0);

  VAO1.unbind();
  VBO1.unbind();
  EBO1.unbind();

  // Render loop
  while (!glfwWindowShouldClose(window))
  {
    processInput(window);

    // Render here
    glClearColor(0.2f,0.1f,0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Drawing Triangle 
    default_shader.use();

    //glBindVertexArray(VAO);
    VAO1.bind();

    glDrawElements(GL_TRIANGLES, 9, GL_UNSIGNED_INT, 0);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // Cleanup
  VAO1.cleanup();
  VBO1.cleanup();
  EBO1.cleanup();
  default_shader.cleanup();

  glfwTerminate();
  return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
  glViewport(0,0,width,height);
}

void processInput(GLFWwindow* window)
{
  if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
}
