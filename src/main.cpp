#include <cmath>
#include <iostream>
#include <vector>

#include <glad/glad.h>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>

#include "rendering/EBO.h"
#include "rendering/VAO.h"
#include "rendering/VBO.h"
#include "rendering/camera.h"
#include "rendering/mesh.h"
#include "rendering/shader.h"

#include "tree/grid.h"
#include "tree/skeleton.h"
#include "tree/strands.h"
#include "tree/implicit.h"

#include "const.h" // TODO TEMPORARY SOLUTION
#include "util/stopwatch.h"
#include "util/geometry.h"

// Default screen dimensions
const unsigned int DEFAULT_WIDTH = 800;
const unsigned int DEFAULT_HEIGHT = 600;
// Current screen dimensions
unsigned int width = DEFAULT_WIDTH;
unsigned int height = DEFAULT_HEIGHT;

GLFWwindow *openGLInit();
void framebuffer_size_callback(GLFWwindow *window, int w, int h);
void processInput(GLFWwindow *window);

Camera camera(FOCUS, DISTANCE, glm::pi<float>(), 0.0f, width, height);

int main(int argc, char *argv[]) {
    // Validating input
    if (argc != 2) {
        std::cout << "input a tree file" << std::endl;
        return -1;
    }
    srand(time(NULL));

    // Ready window
    GLFWwindow *window = openGLInit();

    // Readying Shaders
    Shader flat_shader("resources/shaders/flat.vert",
            "resources/shaders/flat.frag");
    Shader shader("resources/shaders/default.vert",
            "resources/shaders/default.frag");

    Stopwatch sw; // Performance stopwatch
                  // Creating tree
    sw.start();
    Skeleton tree(argv[1]);
    sw.stop();

    // Grid
    Grid gr(tree, 0.01f, 1);
    //Grid gr(glm::ivec3(40,40,40),0.05f,glm::vec3(-1.f,0.f,-1.f));
    //Grid gr(glm::ivec3(400,400,400),0.005f,glm::vec3(-1.f,0.f,-1.f));

    // Tree detail
    Strands detail(tree, gr);
    sw.start();
    detail.add_strands(tree.leafs_size());
    //detail.add_strand(0);
    sw.stop();
    // TEST OCCUPY
    /*
    MetaBalls df(0.04);
    std::vector<glm::vec3> test_path = {
          glm::vec3(0.2,0.1,0),
          glm::vec3(0.2,0.15,0),
          glm::vec3(0.23,0.2,0),
          glm::vec3(0.19,0.25,0),
        //glm::vec3(0.2, 0.1, 0),
        //glm::vec3(0.2, 0.2, 0),
    };
    gr.occupy_path(test_path, 1.f);
    */

    //glm::vec3 test_pos(0.3,0.3,-0.1);
    //glm::vec3 closest = closest_on_path(test_pos,test_path,0,4);
    //gr.occupy_line(test_pos, closest,1.f);
    //gr.fill_path(test_path, df);

    //std::cout<<"eval: "<<df.eval(glm::vec3(0.6,0.6,0),test_path,0)<<std::endl;
    //gr.occupy_pos(glm::vec3(0,0.4,0),1.f);

    // Creating Meshes
    sw.start();
    // Mesh tree_skelly = tree.get_mesh();
    // Mesh detail_geom = detail.get_mesh();
    Mesh bound_geom = gr.get_bound_geom();
    //Mesh grid_geom = gr.get_grid_geom();
    Mesh occupy_geom = gr.get_occupied_geom(0.01);
    //Mesh occupy_dots = gr.get_occupied_geom_points(0.3);
    sw.stop();

    // GROUND PLANE
    glm::vec3 ground_color = glm::vec3(0, 0.65, 0);
    std::vector<Vertex> ground_verts{
        Vertex{glm::vec3(50, 0, 50), ground_color},
            Vertex{glm::vec3(50, 0, -50), ground_color},
            Vertex{glm::vec3(-50, 0, 50), ground_color},
            Vertex{glm::vec3(-50, 0, -50), ground_color}};
    std::vector<GLuint> ground_indices{0, 1, 3, 0, 3, 2};
    Mesh ground(ground_verts, ground_indices);

    // Render loop
    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        // Render here
        glClearColor(SKY_COLOR[0], SKY_COLOR[1], SKY_COLOR[2], SKY_COLOR[3]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Draw the meshes here
        // tree_skelly.draw(flat_shader,camera, GL_LINES);
        // detail_geom.draw(flat_shader,camera, GL_LINES);
        bound_geom.draw(flat_shader,camera, GL_LINES);
        //grid_geom.draw(flat_shader,camera, GL_LINES);
        occupy_geom.draw(shader, camera, GL_TRIANGLES);
        // occupy_dots.draw(flat_shader,camera, GL_POINTS);
        ground.draw(shader, camera, GL_TRIANGLES);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    shader.cleanup();

    glfwTerminate();
    return 0;
}

GLFWwindow *openGLInit() {
    // GLFW init
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        glfwTerminate();
        exit(-1);
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Window Creation
    GLFWwindow *window = glfwCreateWindow(DEFAULT_WIDTH, DEFAULT_HEIGHT,
            "tree strands (DEBUG)", NULL, NULL);

    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(-1);
    }
    glfwMakeContextCurrent(window);

    // GLAD init
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        exit(-1);
    }

    // readying viewport
    glViewport(0, 0, width, height);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // OpenGL drawing settings
    // glPointSize(2.f);
    // glLineWidth(4.f);
    // glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    glEnable(GL_DEPTH_TEST);

    return window;
}

void framebuffer_size_callback(GLFWwindow *window, int w, int h) {
    width = w;
    height = h;
    glViewport(0, 0, width, height);
    camera.set_aspect_ratio(width, height);
}

#define SENS 0.5f
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    // Camera Movement
    // RESET
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) // RESET VIEW
        camera.reset();
    // ROTATE AROUND FOCUS
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) // ROTATE UP
        camera.rotate_vert(0.1f * SENS);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) // ROTATE DOWN
        camera.rotate_vert(-0.1f * SENS);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) // ROTATE LEFT
        camera.rotate_horz(-0.1f * SENS);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) // ROTATE RIGHT
        camera.rotate_horz(0.1f * SENS);
    // ZOOM BOOM ARM
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) // ZOOM OUT
        camera.move_distance(0.2f * SENS);
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) // ZOOM IN
        camera.move_distance(-0.2f * SENS);
    // PAN FOCUS
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) // PAN UP
        camera.move_focus(glm::vec3(0.f, 0.05f, 0.f) * SENS);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) // PAN DOWN
        camera.move_focus(glm::vec3(0.f, -0.05f, 0.f) * SENS);
}
