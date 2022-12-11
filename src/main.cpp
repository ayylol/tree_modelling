#include <iostream>
#include <vector>
#include <cmath>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>

#include "rendering/shader.h"
#include "rendering/VAO.h"
#include "rendering/VBO.h"
#include "rendering/EBO.h"
#include "rendering/camera.h"
#include "rendering/mesh.h"

#include "tree/grid.h"
#include "tree/skeleton.h"
#include "tree/strands.h"

#include "const.h"  // TODO TEMPORARY SOLUTION

// Default screen dimensions
const unsigned int DEFAULT_WIDTH = 800;
const unsigned int DEFAULT_HEIGHT = 600;
// Current screen dimensions 
unsigned int width = DEFAULT_WIDTH;
unsigned int height = DEFAULT_HEIGHT;

void framebuffer_size_callback(GLFWwindow* window, int w, int h);
void processInput(GLFWwindow* window);

Camera camera(FOCUS,DISTANCE,glm::pi<float>(),0.0f, width, height);

int main(int argc, char* argv[]) 
{
    // Validating input
    if (argc != 2 ){
        std::cout<<"input a tree file"<<std::endl;
        return -1;
    }
    srand(time(NULL));
    // TODO: move this to a function
    // OpenGL initialization
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
    
    // readying viewport
    glViewport(0,0,width,height);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Readying Shaders 
    Shader shader("resources/shaders/default.vert", "resources/shaders/default.frag");

    glEnable(GL_DEPTH_TEST);
    glPointSize(2.f);
    //glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    // Done OpenGL initialization

    // Grid
    Grid gr(DIMENSIONS, SCALE, CENTER);

    // Creating tree
    Skeleton tree(argv[1]);

    // Tree detail
    Strands detail(tree, gr);
    //detail.add_strand(4);
    detail.add_strands(526);

    Mesh tree_skelly = tree.get_mesh();
    //Mesh detail_geom = detail.get_mesh();
    Mesh bound_geom = gr.get_bound_geom();
    Mesh occupy_geom = gr.get_occupied_geom();
    //Mesh occupy_dots = gr.get_occupied_geom_points();

    // Render loop
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        // Render here
        glClearColor(SKY_COLOR[0],SKY_COLOR[1],SKY_COLOR[2],SKY_COLOR[3]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Draw the meshes here
        tree_skelly.draw(shader,camera, GL_LINES);
        //detail_geom.draw(shader,camera, GL_POINTS);
        bound_geom.draw(shader,camera, GL_LINES);
        occupy_geom.draw(shader,camera, GL_TRIANGLES);
        //occupy_dots.draw(shader,camera, GL_POINTS);

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

#define SENS 0.5f
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
        camera.rotate_vert(0.1f*SENS);
    if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)  // ROTATE DOWN
        camera.rotate_vert(-0.1f*SENS);
    if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)  // ROTATE LEFT
        camera.rotate_horz(-0.1f*SENS);
    if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)  // ROTATE RIGHT
        camera.rotate_horz(0.1f*SENS);
    // ZOOM BOOM ARM
    if(glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)  // ZOOM OUT
        camera.move_distance(0.2f*SENS);
    if(glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)  // ZOOM IN
        camera.move_distance(-0.2f*SENS);
    // PAN FOCUS
    if(glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)  // PAN UP
        camera.move_focus(glm::vec3(0.f,0.05f,0.f)*SENS);
    if(glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)  // PAN DOWN
        camera.move_focus(glm::vec3(0.f,-0.05f,0.f)*SENS);
}
