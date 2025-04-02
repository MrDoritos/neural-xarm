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

namespace {
    glm::ivec4 current_window, initial_window;
    texture_t *textTexture, *mainTexture, *circleTexture;
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
    //void reset();
    //void toggle_fullscreen_state();

    //glm::mat4 viewport_inversion;
}