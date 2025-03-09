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

using vec3_d = glm::vec<3, double>;
using hrc = std::chrono::high_resolution_clock;
using tp = std::chrono::time_point<hrc>;
using dur = std::chrono::duration<double>;

glm::vec<4, int> initial_window;
glm::vec<4, int> current_window;
bool fullscreen;
bool debug_mode = true;
bool debug_pedantic = false;
bool debug_ui = false;
bool model_interpolation = true;
float mouseSensitivity = 0.05f;
float preciseSpeed = 0.1f;
float movementSpeed = 10.0f;
float rapidSpeed = 20.0f;
const GLuint gluninitialized = -1, glfail = -1, glsuccess = GL_NO_ERROR, glcaught = 1;
glm::mat4 viewport_inversion(1.);
GLFWwindow *window;
GLint uni_projection, uni_model, uni_norm, uni_view;
vec3_d robot_target;
tp start, end;
dur duration;
int frameCount = 0;
double fps;
double lastTime, deltaTime;
glm::vec4 x_axis(1,0,0,0), y_axis(0,1,0,0), z_axis(0,0,1,0);

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

