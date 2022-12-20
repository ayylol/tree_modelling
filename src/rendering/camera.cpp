#include "rendering/camera.h"
#include <iostream>

//
Camera::Camera(glm::vec3 start_focus, float start_distance, float start_theta, float start_phi, int start_width, int start_height) :
    default_focus(start_focus),
    default_distance(start_distance),
    default_theta(start_theta),
    default_phi(start_phi)
    //aspect_ratio(aspect_ratio)
{
    set_aspect_ratio(start_width, start_height);
    reset();
}

// Movement 
void Camera::reset()
{
    focus = default_focus;
    distance = default_distance; 
    theta = default_theta;
    phi = default_phi;
}

void Camera::set_aspect_ratio(int width, int height)
{
    aspect_ratio = (float)width/height;
}

void Camera::move_focus(glm::vec3 move)
{
    focus+=move;
}
void Camera::move_distance(float amount)
{
    distance = std::max(distance+amount, 0.f);
}
void Camera::rotate_vert(float amount)
{
    phi = std::clamp(phi+amount, -max_phi,max_phi);
}
void Camera::rotate_horz(float amount)
{
    theta += amount;
}

// Get camera transform matrix
glm::mat4 Camera::get_matrix() const
{
    /*
    glm::quat rot_mat(glm::vec3(phi,theta,0.f));
    glm::vec3 camera_loc = glm::vec3(rot_mat*glm::vec4(0.f,0.f,-distance,1.f))+focus;
    */
    glm::vec3 camera_loc = get_position();

    glm::mat4 view = glm::lookAt(camera_loc, focus, glm::vec3(0.f,1.f,0.f));
    glm::mat4 proj = glm::perspective(glm::radians(50.0f), aspect_ratio, 0.01f, 100.0f);

    return proj*view;
}
// Get camera position
glm::vec3 Camera::get_position() const
{
    glm::quat rot_mat(glm::vec3(phi,theta,0.f));
    glm::vec3 camera_loc = glm::vec3(rot_mat*glm::vec4(0.f,0.f,-distance,1.f))+focus;
    return camera_loc;
}
