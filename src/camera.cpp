#include "camera.h"
#include "util.h"

void camera_t::calculate_normals() {
    yaw = util::wrap(yaw, -180, 180);
    pitch = util::clip(pitch, -89.9f, 89.9f);

    front = glm::normalize(glm::vec3(
        std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch)),
        std::sin(glm::radians(pitch)),
        std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch))
    ));

    right = glm::normalize(glm::cross(front, up));
}

void camera_t::mousePress(GLFWwindow *window, int button, int action, int mods) {
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

void camera_t::mouseMove(GLFWwindow *window, double x, double y) {
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

    //pitch = std::max(std::min(pitch, 89.9f), -89.9f);

    calculate_normals();
}

void camera_t::joystick_move(GLFWwindow *window, float pitch, float yaw) {
    this->yaw += yaw;
    this->pitch += pitch;
    this->pitch = std::max(std::min(this->pitch, 89.9f), -89.9f);

    calculate_normals();
}

void camera_t::keyboard(GLFWwindow *window, float deltaTime) {
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

    glm::vec3 front = this->front; front.y = 0;
    glm::vec3 right = this->right; right.y = 0;
    front = glm::normalize(front);
    right = glm::normalize(right);

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