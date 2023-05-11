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

// Toggles
bool view_mesh = true, 
     view_volume = false, 
     view_strands = false,
     view_normals = false,
     view_skeleton = false;

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
    Mesh skeleton_geom = tree.get_mesh();
    Mesh tree_geom = gr.get_occupied_geom(surface_val);
    Mesh volume_geom = gr.get_occupied_geom_points(0.0f);
    Mesh strands_geom = detail.get_mesh();
    Mesh normals_geom = gr.get_normals_geom(surface_val);
    Mesh bound_geom = gr.get_bound_geom();
    sw.stop();
    gr.export_data("data.txt");

    // GROUND PLANE
    glm::vec3 ground_color = glm::vec3(0, 0.3, 0.02);
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
        //bound_geom.draw(flat_shader, *camera, GL_LINES);
        if (view_mesh) tree_geom.draw(shader, *camera, GL_TRIANGLES);
        if (view_volume) volume_geom.draw(flat_shader, *camera, GL_POINTS);
        if (view_strands) strands_geom.draw(flat_shader, *camera, GL_LINES);
        if (view_normals) normals_geom.draw(flat_shader, *camera, GL_LINES);
        if (view_skeleton) skeleton_geom.draw(flat_shader, *camera, GL_LINES);
        //ground.draw(shader, *camera, GL_TRIANGLES);

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
    glPointSize(2.f);
    //glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
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
bool pressed1 = false, pressed2 = false, pressed3 = false, pressed4 = false, pressed5 = false;
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
    // Mesh view modes 
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS && !pressed1){ // Toggle
        view_mesh = !view_mesh;
        pressed1 = true;
    }
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_RELEASE && pressed1) pressed1 = false;

    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS && !pressed2){ // Toggle
        view_volume = !view_volume;
        pressed2 = true;
    }
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_RELEASE && pressed2) pressed2 = false;

    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS && !pressed3){ // Toggle
        view_strands = !view_strands;
        pressed3 = true;
    }
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_RELEASE && pressed3) pressed3 = false;

    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS && !pressed4){ // Toggle
        view_normals = !view_normals;
        pressed4 = true;
    }
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_RELEASE && pressed4) pressed4 = false;

    if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS && !pressed5){ // Toggle
        view_skeleton = !view_skeleton;
        pressed5 = true;
    }
    if (glfwGetKey(window, GLFW_KEY_5) == GLFW_RELEASE && pressed5) pressed5 = false;
}
