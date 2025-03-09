#pragma once

#include <glm/gtc/matrix_transform.hpp>

#include "common.h"

struct camera_t;

struct camera_t {
    camera_t(glm::vec3 position = {0,0,0.}, float pitch = 0., float yaw = 0., float fov = 90.) {
        position = glm::vec3(0.0f, 0.0f, 0.0f);
        up = glm::vec3(0.0f, 1.0f, 0.0f);
        front = glm::vec3(0.0f, 0.0f, -1.0f);
        right = glm::vec3(1.0f, 0.0f, 0.0f);
        fov = 90.0f;
        near = 0.1f;
        far = 1000.0f;
        pitch = 0.0f;
        yaw = 0.0f;
        last_mouse[0] = 0.0f;
        last_mouse[1] = 0.0f;

        this->position = position;
        this->pitch = pitch;
        this->yaw = yaw;
        this->fov = fov;

        calculate_normals();
    }

    glm::vec3 position, up, front, right;
    float fov, near, far, pitch, yaw;
    double last_mouse[2];
    bool no_mouse = true;
    bool interactive = false;

    glm::mat4 get_view_matrix() {
        return glm::lookAt(position, position + front, up);
    }

    glm::mat4 get_projection_matrix() {
        return glm::perspective(glm::radians(this->fov),
                                (float) current_window[2] / current_window[3], 
                                this->near, this->far) * viewport_inversion;
    }

    void calculate_normals() {
        front = glm::normalize(glm::vec3(
            std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch)),
            std::sin(glm::radians(pitch)),
            std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch))
        ));

        right = glm::cross(front, up);
    }

    bool isInteractive() {
        return interactive;
    }

    void mousePress(GLFWwindow *window, int button, int action, int mods) {
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            interactive = !interactive;
        } else
        if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS && interactive) {
            interactive = false;
        }

        if (interactive) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            no_mouse = true;
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    void mouseMove(GLFWwindow *window, double x, double y) {
        if (!isInteractive())
            return;

        double dx = x - last_mouse[0];
        double dy = last_mouse[1] - y;

        last_mouse[0] = x;
        last_mouse[1] = y;

        if (no_mouse) {
            no_mouse = false;
            return;
        }

        yaw += mouseSensitivity * dx;
        pitch += mouseSensitivity * dy;

        pitch = std::max(std::min(pitch, 89.9f), -89.9f);

        calculate_normals();
    }

    void joystick_move(GLFWwindow *window, float pitch, float yaw) {
        this->yaw += yaw;
        this->pitch += pitch;
        this->pitch = std::max(std::min(this->pitch, 89.9f), -89.9f);

        calculate_normals();
    }

    void keyboard(GLFWwindow *window, float deltaTime) {
        if (!isInteractive())
            return;

        float movementFactor = movementSpeed;
        bool precise = (glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);
        bool rapid = (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS);

        if (rapid && precise) {
            
        } else
        if (rapid) {
            movementFactor = rapidSpeed;
        } else
        if (precise) {
            movementFactor = preciseSpeed;
        }

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            position += front * deltaTime * movementFactor;
        }

        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            position -= front * deltaTime * movementFactor;
        }

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            position -= right  * deltaTime * movementFactor;
        }

        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            position += right * deltaTime * movementFactor;
        }

        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            position += up * deltaTime * movementFactor;
        }

        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            position -= up * deltaTime * movementFactor;
        }
    }
};
