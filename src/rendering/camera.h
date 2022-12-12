#pragma once

#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/constants.hpp>

const float max_phi = glm::half_pi<float>()-0.1f;

class Camera
{
    public:
        Camera(glm::vec3 start_focus, float distance, float start_theta, float start_phi, int start_width, int start_height);

        // Movement 
        void reset();
        void set_aspect_ratio(int width, int height);
        void move_focus(glm::vec3 move);
        void move_distance(float amount);
        void rotate_vert(float amount);
        void rotate_horz(float amount);

        // Get camera transform matrix
        glm::vec3 get_position() const;
        glm::mat4 get_matrix() const;
    private:
        float aspect_ratio;
        glm::vec3 focus;  // look at point
        float distance;   // Distance from look at point
        float theta;      // Horizontal Angle
        float phi;        // Vertical Angle

        glm::vec3 default_focus;
        float default_distance;
        float default_theta;
        float default_phi; 
};
