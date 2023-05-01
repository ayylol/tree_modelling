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

#include <nlohmann/json.hpp>

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

#include "util/stopwatch.h"
#include "util/geometry.h"

#define SKY_COLOR glm::vec4(0.529,0.808,0.922,1.0)

using json = nlohmann::json;
// Default screen dimensions
const unsigned int DEFAULT_WIDTH = 800;
const unsigned int DEFAULT_HEIGHT = 600;
// Current screen dimensions
unsigned int width = DEFAULT_WIDTH;
unsigned int height = DEFAULT_HEIGHT;

GLFWwindow *openGLInit();
void framebuffer_size_callback(GLFWwindow *window, int w, int h);
void processInput(GLFWwindow *window);

Camera* camera;

int main(int argc, char *argv[]) {

    // Validating input
    if (argc != 2) {
        std::cerr << "input an options file" << std::endl;
        return 1;
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

    // Parse options
    json opt_data = json::parse(std::ifstream(argv[1]));


    // Creating tree
    sw.start();
    Skeleton tree(opt_data);
    sw.stop();

    // Grid
    Grid gr = Grid(tree, 0.1f, opt_data.at("grid_scale"));

    // Make camera according to grid
    Camera cam(gr.get_center(), 2.5f*(gr.get_center()-gr.get_backbottomleft()).z, width, height);
    camera = &cam;

    // Tree detail
    Implicit *df;
    if(opt_data.at("implicit").at("type")=="metaballs"){
      df = new MetaBalls(opt_data.at("implicit"));
    }else if(opt_data.at("implicit").at("type")=="blinn"){
      df = new Blinn(opt_data.at("implicit"));
    }else{
      std::cerr << "did not recognize implicit type" << std::endl;
      return 1;
    }
    Strands detail(tree, gr, *df);
    sw.start();
    detail.add_strands(opt_data.at("strands"));
    sw.stop();
    delete df;

    // Creating Meshes
    sw.start();
    float surface_val = opt_data.at("mesh_iso");
    Mesh tree_skelly = tree.get_mesh();
    Mesh detail_geom = detail.get_mesh();
    Mesh bound_geom = gr.get_bound_geom();
    Mesh grid_geom = gr.get_grid_geom();
    Mesh occupy_geom = gr.get_occupied_geom(surface_val);
    Mesh occupy_dots = gr.get_occupied_geom_points(surface_val);
    Mesh strands = detail.get_mesh();
    sw.stop();
    gr.export_data("data.txt");

    // GROUND PLANE
    glm::vec3 ground_color = glm::vec3(0, 0.6, 0.02);
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
        // tree_skelly.draw(flat_shader, *camera, GL_LINES);
        // detail_geom.draw(flat_shader, *camera, GL_LINES);
        bound_geom.draw(flat_shader, *camera, GL_LINES);
        // grid_geom.draw(flat_shader, *camera, GL_LINES);
        occupy_geom.draw(shader, *camera, GL_TRIANGLES);
        // occupy_dots.draw(flat_shader, *camera, GL_POINTS);
        // strands.draw(flat_shader, *camera, GL_LINES);
        ground.draw(shader, *camera, GL_TRIANGLES);

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
    glLineWidth(4.f);
    // glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    glEnable(GL_DEPTH_TEST);

    return window;
}

void framebuffer_size_callback(GLFWwindow *window, int w, int h) {
    width = w;
    height = h;
    glViewport(0, 0, width, height);
    camera->set_aspect_ratio(width, height);
}

#define SENS 0.5f
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    // Camera Movement
    // RESET
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) // RESET VIEW
        camera->reset();
    // ROTATE AROUND FOCUS
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) // ROTATE UP
        camera->rotate_vert(0.1f * SENS);
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) // ROTATE DOWN
        camera->rotate_vert(-0.1f * SENS);
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) // ROTATE LEFT
        camera->rotate_horz(-0.1f * SENS);
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) // ROTATE RIGHT
        camera->rotate_horz(0.1f * SENS);
    // ZOOM BOOM ARM
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) // ZOOM OUT
        camera->move_distance(0.2f * SENS);
    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) // ZOOM IN
        camera->move_distance(-0.2f * SENS);
    // PAN FOCUS
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) // PAN UP
        camera->move_focus(glm::vec3(0.f, 0.05f, 0.f) * SENS);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) // PAN DOWN
        camera->move_focus(glm::vec3(0.f, -0.05f, 0.f) * SENS);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) // PAN LEFT
        camera->pan_side(-0.05f * SENS);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) // PAN RIGHT
        camera->pan_side(0.05f * SENS);
    
}
