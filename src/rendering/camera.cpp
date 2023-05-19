#include "rendering/camera.h"
#include "glm/geometric.hpp"
#include <cmath>
#include <iostream>
#include <string>

//
Camera::Camera(glm::vec3 start_focus, float start_distance, float start_theta,
               float start_phi, int start_width, int start_height)
    : default_focus(start_focus), default_distance(start_distance),
      default_theta(start_theta), default_phi(start_phi)
// aspect_ratio(aspect_ratio)
{
  set_aspect_ratio(start_width, start_height);
  reset();
}

// Movement
void Camera::reset() {
  focus = default_focus;
  distance = default_distance;
  theta = default_theta;
  phi = default_phi;
}

void Camera::set_aspect_ratio(int width, int height) {
  aspect_ratio = (float)width / height;
}

void Camera::move_focus(glm::vec3 move) { focus += move; }

void Camera::pan_fwd(float amount){
  glm::vec3 dir = glm::normalize(get_matrix() * glm::vec4(0, 0, 1, 0));
  move_focus(dir * amount);
}
void Camera::pan_side(float amount) {
  glm::vec3 dir = get_matrix() * glm::vec4(1, 0, 0, 0);
  dir.y = 0;
  dir = glm::normalize(dir);
  move_focus(dir * amount);
}

#define MAX_DIST 12.f
#define MIN_DIST 0.1f
void Camera::move_distance(float amount) {
  distance = fmin(fmax(distance + amount, MIN_DIST),MAX_DIST);
}
void Camera::rotate_vert(float amount) {
  phi = std::clamp(phi + amount, -max_phi, max_phi);
}
void Camera::rotate_horz(float amount) { theta = std::fmod(theta + amount, 6.24f); }

// Get camera transform matrix
glm::mat4 Camera::get_matrix() const {
  glm::vec3 camera_loc = get_position();

  glm::mat4 view = glm::lookAt(camera_loc, focus, glm::vec3(0.f, 1.f, 0.f));
  glm::mat4 proj =
      glm::perspective(glm::radians(50.0f), aspect_ratio, 0.01f, 100.0f);

  return proj * view;
}
// Get camera position
glm::vec3 Camera::get_position() const {
  glm::quat rot_mat(glm::vec3(phi, theta, 0.f));
  glm::vec3 camera_loc =
      glm::vec3(rot_mat * glm::vec4(0.f, 0.f, -distance, 1.f)) + focus;
  return camera_loc;
}

std::string Camera::to_string(){
  return "focus: (" + std::to_string(focus.x) + " " + std::to_string(focus.y) +
         " " + std::to_string(focus.z) + ")\ndistance: " + std::to_string(distance) +
         "\ntheta:" + std::to_string(theta) + "\nphi: " + std::to_string(phi);
}
nlohmann::json Camera::get_json(){
  using json = nlohmann::json;
  json cam_data=json::parse("{ \"focus\": [0.0,0.0,0.0], \"dist\": 4.0, \"theta\": 0.0, \"phi\": 0.0 }");
  cam_data.at("focus") = {focus.x,focus.y,focus.z};
  cam_data.at("dist") = distance;
  cam_data.at("theta") = theta;
  cam_data.at("phi") = phi;
  return cam_data;
}
