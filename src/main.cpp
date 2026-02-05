#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

#include <glad/glad.h>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>

#include <nlohmann/json.hpp>
#include <omp.h>

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


//#define SKY_COLOR glm::vec4(0.529,0.808,0.922,1.0)
//#define SKY_COLOR glm::vec4(0,0,0,1.0)
#define SKY_COLOR glm::vec4(1,1,1,1.0)

using json = nlohmann::json;
// Default screen dimensions
const unsigned int DEFAULT_WIDTH = 800;
const unsigned int DEFAULT_HEIGHT = 600;
// Current screen dimensions
unsigned int width = DEFAULT_WIDTH;
unsigned int height = DEFAULT_HEIGHT;

double mouse_x, mouse_y;
double scroll_amount=0.f;

GLFWwindow *openGLInit();
void framebuffer_size_callback(GLFWwindow *window, int w, int h);
void processInput(GLFWwindow *window);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void save_image();
void save_mesh(Mesh<Vertex> mesh);

std::vector<Camera> cameras;
size_t curr_cam = 0;
void cycle_camera(int dir) { 
    curr_cam += dir; 
    if (curr_cam<0) curr_cam = cameras.size()-1;
    if (curr_cam>=cameras.size()) curr_cam = 0;
}
#define CAMERA cameras[curr_cam]

std::string image_prefix = "tree";

// Toggles
bool view_mesh = true, 
     view_volume = false, 
     view_strands = true,
     view_normals = false,
     view_skeleton = false,
     view_ground = false,
     view_bound = false,
     interactive = true,
     next_stage = false,
     gen_geom = false,
     geom_generated = false;

bool export_mesh = false;

bool reset_strands = false;
float strands_start = 0.0f,
      strands_end = 1.0f,
      node_vis_f = 0.5f;

int main(int argc, char *argv[]) {
    // Validating input
    if (argc != 2) {
        std::cerr << "input an options file" << std::endl;
        return 1;
    }

    int num_threads = 24;
    omp_set_num_threads(num_threads);

    srand(time(NULL));

    // Ready window
    GLFWwindow *window = openGLInit();

    // Readying Shaders 
    Shader flat_shader("resources/shaders/flat.vert", "resources/shaders/flat.frag");
    Shader shader("resources/shaders/default.vert",
            "resources/shaders/default.frag");

    Stopwatch sw; // Performance stopwatch
 
#define STOPWATCH(ACTION, ...)                                                 \
    std::cout << ACTION << "..." << std::endl;                                 \
    TIME(sw, __VA_ARGS__);                                                     \
    std::cout << std::endl

    // Parse options
    std::filesystem::path file(argv[1]);
    auto option_file = std::ifstream(file);
    json opt_data = json::parse(option_file);
    opt_data["path"]=file.remove_filename();

    // Creating tree
    STOPWATCH("Parsing Skeleton", Skeleton tree(opt_data););

    // Grid
    STOPWATCH("Initializing Grid",
            Grid gr = Grid(tree, 0.01f, opt_data.at("grid_scale"));
            Grid texture_grid = Grid(tree, 0.01f, opt_data.at("grid_scale"));
            );
    // Make camera according to grid
    cameras.push_back(Camera(gr.get_center(), 2.5f*(gr.get_center()-gr.get_backbottomleft()).z, width, height));
    if (opt_data.contains("cameras")){
        for (auto cam_data : opt_data.at("cameras")){
            cameras.push_back(Camera(cam_data, width, height));
        }
    }
    // Set up output file
    if (opt_data.contains("image_path")) {
        image_prefix = opt_data["image_path"];
    }
    image_prefix=std::string(opt_data["path"])+"output/"+image_prefix;
    if (!std::filesystem::is_directory(std::filesystem::path(image_prefix).remove_filename())){
      std::string mkdir_cmd = std::string("mkdir -p ")+
        std::string(std::filesystem::path(image_prefix).remove_filename());
      system(mkdir_cmd.c_str());
    }

    // Tree detail
    STOPWATCH("Adding Strands",
        Strands detail(tree, gr, texture_grid, opt_data);
        detail.add_stage();
        );

    // Creating Meshes
    STOPWATCH("Creating Skeleton Mesh", Mesh skeleton_geom = tree.get_mesh(););
    //STOPWATCH("Getting Occupied Volume", Mesh volume_geom = gr.get_occupied_geom_points(0.0f););
    STOPWATCH("Getting Strand", 
        Mesh strands_geom = detail.get_mesh();
        Mesh tstrands_geom = detail.get_mesh(0.f,1.f,Strands::Texture);
        Mesh node_vis = detail.visualize_node(0,0);
        Mesh searchpoint_vis = detail.visualize_searchpoint(0);
        Mesh keypoint_vis = detail.visualize_keypoints(0);
    );
    STOPWATCH("Getting Bounds", Mesh bound_geom = gr.get_bound_geom(););

    float surface_val = opt_data.at("mesh_iso");
    Mesh<Vertex> tree_geom = Mesh(std::vector<Vertex>(), std::vector<GLuint>());
    //Mesh<Vertex> tree_geom=gr.get_occupied_geom(surface_val, texture_grid);
    //Mesh<Vertex> tree_geom=gr.get_occupied_voxels(surface_val);
    //STOPWATCH("Getting Normals", Mesh<VertFlat> normals_geom = gr.get_normals_geom(surface_val););

    if (opt_data.contains("save_mesh") && opt_data.at("save_mesh")){
        STOPWATCH("Exporting Data", 
                    save_mesh(tree_geom);
                );
    }

    // GROUND PLANE
    glm::vec3 ground_color = glm::vec3(0, 0.3, 0.02);
    std::vector<Vertex> ground_verts{
        Vertex{glm::vec3(50,  0, 50), ground_color},
        Vertex{glm::vec3(50,  0, -50), ground_color},
        Vertex{glm::vec3(-50, 0, 50), ground_color},
        Vertex{glm::vec3(-50, 0, -50), ground_color}};
    std::vector<GLuint> ground_indices{0, 1, 3, 0, 3, 2};
    Mesh ground(ground_verts, ground_indices);

    // Toggle interactive mode
    if (opt_data.contains("interactive_mode")) {
        interactive = opt_data.at("interactive_mode");
        cycle_camera(1);
    }
    bool done_screenshots = false;


    // Render loop
    while ((interactive && !glfwWindowShouldClose(window))||
            (!interactive && !done_screenshots)) {
        if (next_stage){
          STOPWATCH("Adding Strands",
              if (detail.add_stage()>=0){
              strands_geom=detail.get_mesh();
              tstrands_geom=detail.get_mesh(0.f,1.f,Strands::Texture);
              }
          );
          next_stage=false;
          geom_generated=false;
        }
        if (gen_geom){
          if (!geom_generated){
            STOPWATCH("Polygonizing Isosurface", 
                tree_geom=gr.get_occupied_geom(surface_val, texture_grid);
            );
          }
          gen_geom=false;
          geom_generated=true;
        }
        if (reset_strands){
            strands_geom = detail.get_mesh(strands_start,strands_end);
            node_vis = detail.visualize_node(strands_end,node_vis_f);
            searchpoint_vis = detail.visualize_searchpoint(strands_end);
            keypoint_vis = detail.visualize_keypoints(strands_end);
            reset_strands = false;
        }
        #define SHOW_CAM_POS 0
        if (SHOW_CAM_POS) std::cout<<CAMERA.to_string()<<"\n"<<std::endl;
        if(interactive) processInput(window);

        // Render here
        glClearColor(SKY_COLOR[0], SKY_COLOR[1], SKY_COLOR[2], SKY_COLOR[3]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Draw the meshes here
        if (view_mesh) {
            tree_geom.draw(shader, CAMERA, GL_TRIANGLES);
        }
        if (view_volume) {
            tstrands_geom.draw(flat_shader, CAMERA, GL_LINES);
            //volume_geom.draw(flat_shader, CAMERA, GL_POINTS);
        }
        if (view_strands) strands_geom.draw(flat_shader, CAMERA, GL_LINES);
        if (view_normals) {
          //normals_geom.draw(flat_shader, CAMERA, GL_LINES);
          node_vis.draw(flat_shader, CAMERA, GL_POINTS);
          node_vis.draw(flat_shader, CAMERA, GL_LINES);
          searchpoint_vis.draw(flat_shader, CAMERA, GL_LINES);
          keypoint_vis.draw(flat_shader, CAMERA, GL_POINTS);
        }
        if (view_skeleton) skeleton_geom.draw(flat_shader, CAMERA, GL_LINES);
        if (view_ground) ground.draw(shader, CAMERA, GL_TRIANGLES);
        if (view_bound) bound_geom.draw(flat_shader, CAMERA, GL_LINES);

        glfwSwapBuffers(window);
        if (interactive) glfwPollEvents();
        else{
            save_image();
            cycle_camera(1);
            if (curr_cam == 0) done_screenshots = true;
        }
        if (export_mesh){
            STOPWATCH("Exporting Data", 
            save_mesh(tree_geom);
            export_mesh = false;
            );
        }
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

    // Initializing mouse position
    glfwGetCursorPos(window, &mouse_x, &mouse_y);
    glfwSetScrollCallback(window, scroll_callback);

    // OpenGL drawing settings
    glPointSize(8.f);
    glLineWidth(2.f);
    //glPolygonMode( GL_BACK, GL_LINE );
    //glPolygonMode( GL_FRONT, GL_POINT );
    //glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    glEnable(GL_DEPTH_TEST);

    return window;
}

void framebuffer_size_callback(GLFWwindow *window, int w, int h) {
    width = w;
    height = h;
    glViewport(0, 0, width, height);
    for (auto cam : cameras){
        cam.set_aspect_ratio(width, height);
    }
}

int images_taken = 0;
void save_image(){
    std::string file_name = image_prefix + "_" + std::to_string(images_taken) + ".png";
    // Make temporary ppm file
    std::string temp_file = "/tmp/tree_image.ppm";
    std::ofstream out(temp_file);
    out<<"P3\n# "<<temp_file<<"\n"<<width<<" "<<height<<"\n255"<<std::endl;
    GLubyte* pixels = new GLubyte[3 * width * height];
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    for (int i = height - 1; i >= 0; i--) {
        for (int j = 0; j < width; j++) {
            int ptr = (i * width + j) * 3;
            out << (int)pixels[ptr] << " " 
                << (int)pixels[ptr + 1] << " " 
                << (int)pixels[ptr + 2] << " ";
        }
    }
    out.close();
    delete [] pixels;
    // Make PNG
    std::string convert_cmd =  "convert " + temp_file + " " + file_name;
    system(convert_cmd.c_str());
    std::cout<<"Saved image: "<<file_name<<std::endl;
    images_taken++;
}

int meshes_exported = 0;
void save_mesh(Mesh<Vertex> mesh){
    std::string file_name = image_prefix +"_"+ std::to_string(meshes_exported) + ".ply";
    std::ofstream out(file_name);
    glm::mat4 pos_transform = glm::scale(glm::vec3(2,2,2));
    glm::mat3 norm_scale = glm::scale(glm::vec3(100,100,100));
    out<<"ply\n";
    out<<"format ascii 1.0\n";
    out<<"element vertex "<<mesh.vertices.size()<<"\n";
    out<<"property float x\n";
    out<<"property float y\n";
    out<<"property float z\n";
    out<<"property float nx\n";
    out<<"property float ny\n";
    out<<"property float nz\n";
    out<<"property uchar red\n";
    out<<"property uchar green\n";
    out<<"property uchar blue\n";
    out<<"element face "<<mesh.indices.size()/3<<"\n";
    out<<"property list uchar int vertex_index\n";
    out<<"end_header\n";
    out<<std::fixed;
    out<<std::setprecision(10);
    for (Vertex vertex : mesh.vertices){
        glm::vec3 pos = pos_transform*glm::vec4(vertex.position,1.f);
        glm::vec3 normal = vertex.normal;
        glm::vec3 col = vertex.color;
        if (glm::any(glm::isnan(normal))){
            //std::cout <<"IS NAN"<<std::endl;
            normal = glm::vec3();
        }
        out <<pos.x<<" "<<pos.y<<" "<<pos.z<<" "
            <<normal.x<<" "<<normal.y<<" "<<normal.z<<" "
            <<(int)(col.r*255)<<" "<<(int)(col.g*255)<<" "<<(int)(col.b*255)<<"\n";
    }
    for (int i=0; i<=mesh.indices.size()-3; i+=3){
        GLuint v0 = mesh.indices[i];
        GLuint v1 = mesh.indices[i+1];
        GLuint v2 = mesh.indices[i+2];
        out<<"3 "<<v0<<" "<<v1<<" "<<v2<<"\n";
    }
    out.close();
    std::cout<<"Exported Mesh: "<<file_name<<std::endl;
    meshes_exported++;
}

#define PANSENS 0.38f
#define ROTSENS 0.5f
#define ZOOMSENS 0.5f
#define MOUSESENS 0.01f
float speed_factor = 1.f;
bool pressed1 = false, pressed2 = false, pressed3 = false, pressed4 = false,
     pressed5 = false, pressed6 = false, pressed7 = false,
     pressedperiod = false, pressedenter = false, pressedga = false,
     pressedn = false, pressedt = false, pressedg = false, pressedbs = false,
     pressedp = false;
void processInput(GLFWwindow *window) {
    // Mouse input
    double mouse_current_x, mouse_current_y;
    glfwGetCursorPos(window, &mouse_current_x, &mouse_current_y);
    double x_diff = mouse_current_x-mouse_x;
    double y_diff = mouse_current_y-mouse_y;
    if ( glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) ){
        CAMERA.rotate_vert(y_diff * ROTSENS * MOUSESENS * speed_factor);
        CAMERA.rotate_horz(-x_diff * ROTSENS * MOUSESENS * speed_factor);
    }
    if ( glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) ){
        CAMERA.move_focus(glm::vec3(0.f, (float)y_diff, 0.f) * PANSENS * MOUSESENS * speed_factor);
        CAMERA.pan_side(-x_diff * PANSENS * MOUSESENS * speed_factor);
    }
    if(scroll_amount!=0.f){
        CAMERA.move_distance(-scroll_amount * ZOOMSENS * speed_factor);
        scroll_amount=0.f;
    }
    mouse_x=mouse_current_x;
    mouse_y=mouse_current_y;
    // Keyboard input
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    // OUTPUT CAMERA JSON
    if (glfwGetKey(window, GLFW_KEY_GRAVE_ACCENT) == GLFW_PRESS && !pressedga){
        std::cout<<CAMERA.get_json()<<std::endl;
        pressedga=true;
    } if (glfwGetKey(window, GLFW_KEY_GRAVE_ACCENT) == GLFW_RELEASE && pressedga) pressedga = false;
    // Camera Movement
    // RESET
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS){ // RESET VIEW
        CAMERA.reset();
        speed_factor=1.f;
    }
    // ROTATE AROUND FOCUS
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) // ROTATE UP
        CAMERA.rotate_vert(0.1f * ROTSENS * speed_factor);
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) // ROTATE DOWN
        CAMERA.rotate_vert(-0.1f * ROTSENS * speed_factor);
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) // ROTATE LEFT
        CAMERA.rotate_horz(-0.1f * ROTSENS * speed_factor);
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) // ROTATE RIGHT
        CAMERA.rotate_horz(0.1f * ROTSENS * speed_factor);
    // ZOOM BOOM ARM
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) // ZOOM OUT
        CAMERA.move_distance(0.2f * ZOOMSENS * speed_factor);
    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) // ZOOM IN
        CAMERA.move_distance(-0.2f * ZOOMSENS * speed_factor);
    // PAN FOCUS
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) // PAN UP
        CAMERA.move_focus(glm::vec3(0.f, 0.05f, 0.f) * PANSENS * speed_factor);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) // PAN DOWN
        CAMERA.move_focus(glm::vec3(0.f, -0.05f, 0.f) * PANSENS * speed_factor);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) // PAN LEFT
        CAMERA.pan_side(-0.05f * PANSENS * speed_factor);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) // PAN RIGHT
        CAMERA.pan_side(0.05f * PANSENS * speed_factor);
    // Change Camera Sens
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) // INCREASE SENS
        speed_factor = std::clamp(speed_factor+0.01f,0.1f,2.f);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) // DECREASE SENS
        speed_factor = std::clamp(speed_factor-0.01f,0.1f,2.f);
    // Cycle camera
    if (glfwGetKey(window, GLFW_KEY_PERIOD) == GLFW_PRESS && !pressedperiod){
        cycle_camera(1);
        pressedperiod = true;
    }
    if (glfwGetKey(window, GLFW_KEY_PERIOD) == GLFW_RELEASE && pressedperiod) pressedperiod = false;

    // SCREENSHOTS
    if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS && !pressedenter){
        save_image();
        pressedenter = true;
    }
    if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_RELEASE && pressedenter) pressedenter = false;

    if (glfwGetKey(window, GLFW_KEY_BACKSLASH) == GLFW_PRESS && !pressedbs){
        export_mesh = true;
        pressedbs = true;
    }
    if (glfwGetKey(window, GLFW_KEY_BACKSLASH) == GLFW_RELEASE && pressedbs) pressedbs = false;

    // Mesh view modes 
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS && !pressed1){ // Toggle
        view_mesh = !view_mesh;
        pressed1 = true;
    }
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_RELEASE && pressed1) pressed1 = false;

    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS && !pressed2){ // Toggle
        view_strands = !view_strands;
        pressed2 = true;
    }
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_RELEASE && pressed2) pressed2 = false;

    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS && !pressed3){ // Toggle
        view_volume = !view_volume;
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

    if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS && !pressed6){ // Toggle
        view_ground = !view_ground;
        pressed6 = true;
    }
    if (glfwGetKey(window, GLFW_KEY_6) == GLFW_RELEASE && pressed6) pressed6 = false;

    if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS && !pressed7){ // Toggle
        view_bound = !view_bound;
        pressed7 = true;
    }
    if (glfwGetKey(window, GLFW_KEY_7) == GLFW_RELEASE && pressed7) pressed7 = false;

    // Next iteration
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS && !pressedn){ // Toggle
        next_stage = true;
        pressedn = true;
    }
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_RELEASE && pressedn) pressedn = false;

    // generate geom
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !pressedp){ // Toggle
        gen_geom = true;
        pressedp = true;
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE && pressedp) pressedp = false;

    // Strand Geom Keybinds
    // Affect Lower bounds
    if (glfwGetKey(window, GLFW_KEY_SEMICOLON) == GLFW_PRESS){
        strands_start = std::max(0.0f, strands_start-0.005f);
        reset_strands = true;
    }
    if (glfwGetKey(window, GLFW_KEY_APOSTROPHE) == GLFW_PRESS){
        if (strands_start == strands_end) strands_end+= 0.00005f;
        strands_start = std::min(strands_end, strands_start+0.005f);
        reset_strands = true;
    }
    // Affect upper bounds
    if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS){
        if (strands_end == strands_start) strands_start-= 0.00005f;
        strands_end = std::max(strands_start, strands_end-0.005f);
        reset_strands = true;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS){
        strands_end = std::min(1.0f, strands_end+0.005f);
        reset_strands = true;
    }
    // Affect node vis
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
        node_vis_f= std::max(node_vis_f-0.002f, 0.f);
        reset_strands = true;
    }
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
        node_vis_f= std::min(node_vis_f+0.002f, 1.f);
        reset_strands = true;
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
    scroll_amount=yoffset;
}
