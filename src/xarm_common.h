#pragma once

#include "common.h"
#include "segment.h"
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

struct RobotShader;
struct kinematics_t;
struct debug_object_t;
struct debug_info_t;
struct joystick_t;
struct robot_interface_t;

namespace {
    glm::ivec4 current_window, initial_window;
    texture_t *textTexture, *mainTexture, *circleTexture;
    shader_t *mainVertexShader, *mainFragmentShader;
    shader_t *textVertexShader, *textFragmentShader;
    gui::UIShader *textProgram;
    RobotShader *mainProgram;
    material_t *robotMaterial;
    camera_t *camera;
    segment_t *sBase, *s6, *s5, *s4, *s3, *s2, *s1;
    std::vector<segment_t*> segments;
    std::vector<segment_t*> visible_segments;
    std::vector<segment_t*> servo_segments;
    std::vector<mesh_t*> meshes;
    debug_object_t *debug_objects;
    ui_text_t *debugInfo;
    ui_toggle_t *debugToggle, *interpolatedToggle, *resetToggle, *resetConnectionToggle, *pedanticToggle;
    ui_slider_t *slider6, *slider5, *slider4, *slider3, *slider2, *slider1, *slider_ambient, *slider_diffuse, *slider_specular, *slider_shininess;
    std::vector<ui_slider_t*> slider_whatever;
    std::vector<ui_slider_t*> servo_sliders;
    ui_element_t *uiHandler, *ui_servo_sliders;
    kinematics_t *kinematics;
    joystick_t *joysticks;
    robot_interface_t *robot_interface;
    gui::frametime_t frametime;
    //void reset();
    //void toggle_fullscreen_state();

    //glm::mat4 viewport_inversion;
}