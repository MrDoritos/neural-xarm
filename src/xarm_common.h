#pragma once

#include "common.h"
#include "camera.h"
#include "texture.h"
#include "mesh.h"
#include "shader_program.h"
#include "shader.h"
#include "ui_element.h"
#include "ui_text.h"
#include "ui_slider.h"
#include "ui_toggle.h"
#include "ui_shader.h"
#include "frametime.h"
#include "util.h"
#include "segment.h"
#include "materials.h"

namespace robot {
    struct RobotInterface;
    struct Kinematics;
    struct Joystick;
    struct JoystickDevice;
}

#include "robot_interface.h"
#include "joystick.h"
#include "segment.h"
#include "kinematics.h"

struct RobotShader;
struct debug_object_t;
struct debug_info_t;

namespace robot {

extern glm::ivec4 current_window, initial_window;
extern texture_t *textTexture, *mainTexture, *circleTexture, *arrowTexture;
extern shader_t *mainVertexShader, *mainFragmentShader;
extern shader_t *textVertexShader, *textFragmentShader;
extern gui::UIShader *textProgram;
extern RobotShader *mainProgram;
extern material_t *robotMaterial;
extern camera_t *camera;
extern robot::Segment *sBase, *s6, *s5, *s4, *s3, *s2, *s1;
extern std::vector<robot::Segment*> segments;
extern std::vector<robot::Segment*> visible_segments;
extern std::vector<robot::Segment*> servo_segments;
extern std::vector<mesh_t*> meshes;
extern debug_object_t *debug_objects;
extern ui_text_t *debugInfo;
extern ui_toggle_t *debugToggle, *interpolatedToggle, *resetToggle, *resetConnectionToggle, *pedanticToggle;
extern ui_slider_t *slider6, *slider5, *slider4, *slider3, *slider2, *slider1, *slider_ambient, *slider_diffuse, *slider_specular, *slider_shininess;
extern std::vector<ui_slider_t*> slider_whatever;
extern std::vector<ui_slider_t*> servo_sliders;
extern ui_element_t *uiHandler, *ui_servo_sliders;
extern robot::Kinematics *kinematics;
extern robot::Joystick *joysticks;
extern robot::RobotInterface *robot_interface;
extern gui::frametime_t frametime;

}