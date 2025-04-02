#include "xarm_common.h"

namespace robot {

glm::ivec4 current_window, initial_window;
texture_t *textTexture, *mainTexture, *circleTexture, *arrowTexture;
shader_t *mainVertexShader, *mainFragmentShader;
shader_t *textVertexShader, *textFragmentShader;
gui::UIShader *textProgram;
RobotShader *mainProgram;
material_t *robotMaterial;
camera_t *camera;
robot::Segment *sBase, *s6, *s5, *s4, *s3, *s2, *s1;
std::vector<robot::Segment*> segments;
std::vector<robot::Segment*> visible_segments;
std::vector<robot::Segment*> servo_segments;
std::vector<mesh_t*> meshes;
debug_object_t *debug_objects;
ui_text_t *debugInfo;
ui_toggle_t *debugToggle, *interpolatedToggle, *resetToggle, *resetConnectionToggle, *pedanticToggle;
ui_slider_t *slider6, *slider5, *slider4, *slider3, *slider2, *slider1, *slider_ambient, *slider_diffuse, *slider_specular, *slider_shininess;
std::vector<ui_slider_t*> slider_whatever;
std::vector<ui_slider_t*> servo_sliders;
ui_element_t *uiHandler, *ui_servo_sliders;
robot::Kinematics *kinematics;
robot::Joystick *joysticks;
robot::RobotInterface *robot_interface;
gui::frametime_t frametime;

}