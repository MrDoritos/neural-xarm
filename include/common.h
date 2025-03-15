#pragma once

#include <iostream>
#include <chrono>
#include <functional>
#include <algorithm>
#include <map>
#include <string>
#include <string.h>
#include <format>

#include <GL/gl.h>
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

using vec3_d = glm::dvec3;
using hrc = std::chrono::high_resolution_clock;
using tp = std::chrono::time_point<hrc>;
using dur = std::chrono::duration<double>;

extern glm::ivec4 initial_window;
extern glm::ivec4 current_window;
extern bool fullscreen;
extern bool debug_mode;
extern bool debug_pedantic;
extern bool debug_ui;
extern bool model_interpolation;
extern float mouseSensitivity;
extern float preciseSpeed;
extern float movementSpeed;
extern float rapidSpeed;
extern const GLuint gluninitialized;
extern const GLuint glfail;
extern const GLuint glsuccess;
extern const GLuint glcaught;
extern glm::mat4 viewport_inversion;
extern GLFWwindow *window;
extern GLint uni_projection;
extern GLint uni_model;
extern GLint uni_norm;
extern GLint uni_view;
extern vec3_d robot_target;
extern glm::vec4 x_axis;
extern glm::vec4 y_axis;
extern glm::vec4 z_axis;

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
void cursor_position_callback(GLFWwindow *window, double x, double y);
void handle_keyboard(GLFWwindow* window, float deltaTime);
void joystick_callback(int jid, int event);
void handle_signal(int sig);

void handle_error(const char *str, int errcode = -1);
void reset();
void destroy();
void hint_exit();
void safe_exit(int errcode = 0);
void set_segments_from_robot();
void set_segments_from_sliders();
void set_sliders_from_segments();
void set_robot_from_segments();
void toggle_fullscreen_state();