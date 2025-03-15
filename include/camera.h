#pragma once

#include <glm/gtc/matrix_transform.hpp>

#include "common.h"

struct camera_t;

struct camera_t {
    inline camera_t(const glm::vec3 position = {0,0,0.}, const float pitch = 0., const float yaw = 0., const float fov = 90.)
    :position(position),pitch(pitch),yaw(yaw),fov(fov),
     up(0.0f,1.0f,0.0f),near(0.1f),far(1000.0f),last_mouse({0})
    { calculate_normals(); }

    glm::vec3 position, up, front, right;
    float fov, near, far, pitch, yaw;
    double last_mouse[2];
    bool no_mouse = true;
    bool interactive = false;

    inline glm::mat4 get_view_matrix() const {
        return glm::lookAt(position, position + front, up);
    }

    inline glm::mat4 get_projection_matrix() const {
        return glm::perspective(glm::radians(this->fov),
                                (float) current_window[2] / current_window[3], 
                                this->near, this->far) * viewport_inversion;
    }

    inline constexpr bool isInteractive() const {
        return interactive;
    }

    void calculate_normals();

    void mousePress(GLFWwindow *window, int button, int action, int mods);

    void mouseMove(GLFWwindow *window, double x, double y);

    void joystick_move(GLFWwindow *window, float pitch, float yaw);

    void keyboard(GLFWwindow *window, float deltaTime);
};
