#include "common.h"

glm::ivec4 initial_window(0);
glm::ivec4 current_window(0);
bool fullscreen = false;
bool debug_mode = false;
bool debug_pedantic = false;
bool debug_ui = false;
bool model_interpolation = true;
float mouseSensitivity = 0.05f;
float preciseSpeed = 0.1f;
float movementSpeed = 10.0f;
float rapidSpeed = 20.0f;
const GLuint gluninitialized = -1;
const GLuint glfail = -1;
const GLuint glsuccess = GL_NO_ERROR;
const GLuint glcaught = 1;
glm::mat4 viewport_inversion(1.);
GLFWwindow *window;
GLint uni_projection;
GLint uni_model;
GLint uni_norm;
GLint uni_view;
vec3_d robot_target;
glm::vec4 x_axis(1,0,0,0);
glm::vec4 y_axis(0,1,0,0);
glm::vec4 z_axis(0,0,1,0);