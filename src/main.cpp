#include <iostream>
#include <vector>
#include <cmath>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>

#include "shader.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"
#include "camera.h"
#include "mesh.h"
#include "grid.h"

// Default screen dimensions
const unsigned int DEFAULT_WIDTH = 800;
const unsigned int DEFAULT_HEIGHT = 600;
// Current screen dimensions 
unsigned int width = DEFAULT_WIDTH;
unsigned int height = DEFAULT_HEIGHT;

void framebuffer_size_callback(GLFWwindow* window, int w, int h);
void processInput(GLFWwindow* window);

// Camera !
Camera camera(glm::vec3(0,0,0),20.f,glm::pi<float>(),0.0f, width, height);

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

    Grid gr(glm::ivec3(5,5,5),1.f);
    
    // TODO: DELETE
    // Test Point
    std::vector<Vertex> verts{Vertex{glm::vec3(0.2, 1.f, 0.f), glm::vec3(1.f,0.f,0.f)}};
    std::vector<GLuint> indices{0};
    Mesh test(verts,indices);
    //
    auto gridcell = gr.pos_to_grid(verts[0].position);
    gr.occupy(verts[0].position, 1);
    std::cout<<gridcell.x<<" "<<gridcell.y<<" "<<gridcell.z<<std::endl;
    std::cout<<gr.get_in_grid(gridcell)<<std::endl;

    // readying viewport
    glViewport(0,0,width,height);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Readying Shaders 
    Shader shader("resources/shaders/default.vert", "resources/shaders/default.frag");

    glEnable(GL_DEPTH_TEST);
    glPointSize(8.f);
    // Render loop
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        // Render here
        glClearColor(0.75f,1.f,1.f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        gr.grid_geom.draw(shader, camera,GL_LINES);
        test.draw(shader,camera,GL_POINTS);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    shader.cleanup();

    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int w, int h)
{
    width = w;
    height = h;
    glViewport(0,0,width,height);
    camera.set_aspect_ratio(width,height);
}

void processInput(GLFWwindow* window)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    // Camera Movement
    // RESET
    if(glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)  // RESET VIEW
        camera.reset();
    // ROTATE AROUND FOCUS
    if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)  // ROTATE UP
        camera.rotate_vert(0.1f);
    if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)  // ROTATE DOWN
        camera.rotate_vert(-0.1f);
    if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)  // ROTATE LEFT
        camera.rotate_horz(-0.1f);
    if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)  // ROTATE RIGHT
        camera.rotate_horz(0.1f);
    // ZOOM BOOM ARM
    if(glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)  // ZOOM OUT
        camera.move_distance(0.5f);
    if(glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)  // ZOOM IN
        camera.move_distance(-0.5f);
    // PAN FOCUS
    if(glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)  // PAN UP
        camera.move_focus(glm::vec3(0.f,0.1f,0.f));
    if(glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)  // PAN DOWN
        camera.move_focus(glm::vec3(0.f,-0.1f,0.f));
    // Not that good since its tied to axis
    /*
       if(glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)  // PAN FORWARD
       camera.move_focus(glm::vec3(0.f,0.f,0.1f));
       if(glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)  // PAN BACK
       camera.move_focus(glm::vec3(0.f,0.f,-0.1f));
       if(glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)  // PAN LEFT
       camera.move_focus(glm::vec3(0.1f,0.f,0.f));
       if(glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)  // PAN RIGHT
       camera.move_focus(glm::vec3(-0.1f,0.f,0.f));
       */
}
