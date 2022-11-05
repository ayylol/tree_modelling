#include <iostream>
#include <cmath>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"

// Default screen dimensions
const unsigned int DEFAULT_WIDTH = 800;
const unsigned int DEFAULT_HEIGHT = 600;
// Current screen dimensions 
unsigned int width = DEFAULT_WIDTH;
unsigned int height = DEFAULT_HEIGHT;

void framebuffer_size_callback(GLFWwindow* window, int w, int h);
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
  GLFWwindow* window = glfwCreateWindow(DEFAULT_WIDTH,DEFAULT_HEIGHT, "tree strands (DEBUG)", NULL, NULL);
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
    // X      Y                         Z     R     G       B
    // FRONT FACE (TR, BR, BL, TL)
     1.f,  1.f, 1.f, 1.f,0.f,0.f,
     1.f, -1.f, 1.f, 0.f,1.f,0.f,
    -1.f, -1.f, 1.f, 0.f,0.f,1.f,
    -1.f,  1.f, 1.f, 0.5f,0.f,0.5f,
    // BACK FACE (TR, BR, BL, TL)
     1.f,  1.f, -1.f, 0.f,0.5f,0.5f, 
     1.f, -1.f, -1.f, 0.5f,0.f,0.5f,
    -1.f, -1.f, -1.f, 0.f,0.f,0.f,
    -1.f,  1.f, -1.f, 1.0f,1.0f,0.5f
	};

	// Indices for vertices order
	GLuint indices[] =
	{
    // Front
    0,1,2,
    2,3,0,
    // Back
    7,6,5,
    5,4,7,
    // Top
    4,0,3,
    3,7,4,
    // Bottom
    5,1,2,
    2,6,5,
    // Left
    3,2,6,
    6,7,3,
    // Right
    4,5,1,
    1,0,4
	};

  // readying viewport
  glViewport(0,0,width,height);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  // Readying Shaders 
  Shader default_shader("resources/shaders/default.vert", "resources/shaders/default.frag");

  VAO VAO1;
  VAO1.bind();

  VBO VBO1(verts, sizeof(verts));
  EBO EBO1(indices, sizeof(indices));

  VAO1.link_attrib(VBO1, 0, 3, GL_FLOAT, 6 * sizeof(float), (void*)0);
  VAO1.link_attrib(VBO1, 1, 3, GL_FLOAT, 6 * sizeof(float), (void*)(3*sizeof(float)));

  VAO1.unbind();
  VBO1.unbind();
  EBO1.unbind();

  // Render loop


  float rotation = 0.0f;
  double prevTime = glfwGetTime();
  glEnable(GL_DEPTH_TEST);
  while (!glfwWindowShouldClose(window))
  {
    processInput(window);

    // Render here
    glClearColor(0.2f,0.1f,0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    default_shader.use();

    // TODO: clean this up
		double crntTime = glfwGetTime();
		if (crntTime - prevTime >= 1 / 60)
		{
			rotation += 0.5f;
			prevTime = crntTime;
		}
    // matrices for 3d TODO: Clean this up
    glm::mat4 model = glm::mat4(1.0f);  
    glm::mat4 view = glm::mat4(1.0f);  
    glm::mat4 proj = glm::mat4(1.0f);  
    model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.f,1.f,0.f));
    view = glm::translate(view, glm::vec3(0.0f,-0.5f,-10.0f));
    proj = glm::perspective(glm::radians(60.0f), (float)width/height, 0.1f, 100.0f);

    GLuint modelLoc = glGetUniformLocation(default_shader.ID, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    int viewLoc = glGetUniformLocation(default_shader.ID, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    int projLoc = glGetUniformLocation(default_shader.ID, "proj");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));

    VAO1.bind();
    glDrawElements(GL_TRIANGLES, sizeof(indices)/sizeof(int), GL_UNSIGNED_INT, 0);

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

void framebuffer_size_callback(GLFWwindow* window, int w, int h)
{
  width = w;
  height = h;
  glViewport(0,0,width,height);
}

void processInput(GLFWwindow* window)
{
  if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
}
