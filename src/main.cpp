#include <iomanip>
#include <thread>
#include <mutex>

#include <signal.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <hidapi/hidapi.h>

#include "common.h"
#include "camera.h"
#include "texture.h"
#include "mesh.h"
#include "shader.h"
#include "shader_program.h"
#include "text.h"
#include "ui_element.h"
#include "ui_text.h"
#include "ui_slider.h"
#include "ui_toggle.h"
#include "frametime.h"
#include "util.h"
#include "segment.h"

struct shader_text_t;
struct shader_materials_t;
struct kinematics_t;
struct debug_object_t;
struct debug_info_t;
struct joystick_t;
struct robot_interface_t;

texture_t *textTexture, *mainTexture;
shader_t *mainVertexShader, *mainFragmentShader;
shader_t *textVertexShader, *textFragmentShader;
shader_text_t *textProgram;
shader_materials_t *mainProgram;
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

struct shader_materials_t : public shader_program_t {
    shader_materials_t(shader_program_t prg)
    :shader_program_t(prg),material(0),light(0) { }

    material_t *material;
    light_t *light;
    glm::vec3 eyePos;

    void set_material(material_t *mat) {
        this->material = mat;
        set_i("material.diffuse", mat->diffuse->textureId);
        set_i("material.specular", mat->specular->textureId);
        set_f("material.shininess", mat->shininess);
    }

    void set_light(light_t *light) {
        this->light = light;
        set_v3("light.position", light->position);
        set_v3("light.ambient", light->ambient);
        set_v3("light.diffuse", light->diffuse);
        set_v3("light.specular", light->specular);
    }

    void set_eye(glm::vec3 eyePos) {
        this->eyePos = eyePos;
        set_v3("eyePos", eyePos);
    }

    void use() override {
        shader_program_t::use();

        if (material)
            material->use();

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glDisable(GL_CULL_FACE);
        glEnable(GL_ALPHA_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
};

struct shader_text_t : public shader_program_t {
    shader_text_t(shader_program_t prg)
    :shader_program_t(prg),mixFactor(0.0) { }

    float mixFactor;
    GLint lastUnit = -1;
    texture_t *lastTexture = nullptr;
    float lastMixFactor = -2;
    glm::mat4 lastProjection;

    void set_sampler(const char *name, texture_t *tex, GLint unit = 0) override {
        //if (lastUnit != unit || lastTexture != tex) {
            shader_program_t::set_sampler(name, tex, unit);
        //    lastUnit = unit;
        //    lastTexture = tex;
        //}
    }

    void use() override {
        shader_program_t::use();

        glDisable(GL_DEPTH_TEST);
        //glEnable(GL_POLYGON_OFFSET_FILL);
        //glDisable(GL_CULL_FACE);        

        if (lastProjection != viewport_inversion) {
            set_m4("projection", glm::mat4(1.) * viewport_inversion);
            lastProjection = viewport_inversion;
        }
        //if (lastMixFactor != mixFactor) {
            set_f("mixFactor", mixFactor);
        //    lastMixFactor = mixFactor;
        //}
    }
};

struct debug_object_t : public mesh_t {
    std::vector<std::pair<glm::vec3, glm::vec3>> _lines;
    std::vector<std::pair<glm::vec3, float>> _spheres;

    void add_line(glm::vec3 origin, glm::vec3 end) {
        _lines.push_back({origin, end});
    }

    void add_sphere(glm::vec3 origin, float radius) {
        _spheres.push_back({origin, radius});
    }

    void clear() override {
        _lines.clear();
        _spheres.clear();

        mesh_t::clear();
    }

    void render() override {
        mainProgram->use();
        mainProgram->set_camera(camera, glm::mat4(1.0f));

        modified = true;
        mesh_t::render();

        glBegin(GL_LINES);
        for (auto &l : _lines) {
            glVertex3f(l.first.x, l.first.y, l.first.z);
            glVertex3f(l.second.x, l.second.y, l.second.z);
        }
        glEnd();

        glBegin(GL_QUADS);
        for (auto &s : _spheres) {
            glm::vec3 ws = s.first;
            float r = s.second * 5;
            glm::vec3 cr = camera->right * glm::vec3(0.5f) * r;
            glm::vec3 cu = camera->up * glm::vec3(0.5f) * r;
            glm::vec3 v1 = ws + cr + cu,
                      v2 = ws + cr - cu, 
                      v3 = ws - cr - cu, 
                      v4 = ws - cr + cu;

            auto glv = [](glm::vec3 p) {
                glVertex3f(p.x, p.y, p.z);
            };

            glv(v1);
            glv(v2);
            glv(v3);
            glv(v4);
        }
        glEnd();

        this->clear();
    }    
};

namespace render {
    void render_vector(debug_object_t *debug_objects, glm::vec3 origin, glm::vec3 end) {
        debug_objects->add_line(origin, end);
    }

    void render_matrix(debug_object_t *debug_objects, glm::vec3 origin, glm::mat4 mat) {
        auto m = glm::vec4(origin[0], origin[1], origin[2], 0);

        for (int i  = 0; i < 3; i++)
            render_vector(debug_objects, origin, m + mat[i]);
    }

    template<typename T = segment_t>
    void render_segment_debug(const T* segment, debug_object_t *debug_objects, const bool &allow_interpolate = true) {
        auto origin = segment->get_origin(allow_interpolate);
        auto seg_vec = segment->get_segment_vector(allow_interpolate);
        auto rot_mat = segment->get_rotation_matrix(allow_interpolate);

        debug_objects->add_sphere(origin, segment->model_scale);
        debug_objects->add_sphere(origin + seg_vec, segment->model_scale);
        render_vector(debug_objects, origin, origin + seg_vec);

        auto tran_rot_mat = glm::translate(rot_mat, origin);

        render_matrix(debug_objects, origin, tran_rot_mat);
    }

    template<typename T = segment_t>
    void render_segment(const T* segment, shader_program_t *program, camera_t *camera, const bool &allow_interpolate = true) {
        if (debug_mode) {
            program->set_camera(camera, glm::mat4(1.0f));
            render_segment_debug(segment, debug_objects, allow_interpolate);
        }
        program->set_camera(camera, segment->get_model_transform(allow_interpolate));
        if (debug_pedantic) {
            program->set_v3("light.ambient", segment->debug_color);
        }
        segment->mesh->render();
    }

    template<typename T = segment_t>
    void render_segments(const std::vector<T*> &segments, shader_program_t *program, camera_t *camera, const bool &allow_interpolate = true) {
        program->use();
        for (T* segment : segments)
            render_segment(segment, program, camera, allow_interpolate);
    }
}

struct kinematics_t {
    bool solve_inverse(vec3_d coordsIn) {
        auto isnot_real = [](float x){
            return (std::isinf(x) || std::isnan(x));
        };

        auto &segments = visible_segments;

        bool calculation_failure = false;

        auto target_coords = coordsIn;
        target_coords.z = -target_coords.z;
        auto target_2d = glm::vec2(target_coords.x, target_coords.z);

        float rot_out[5];

        for (int i = 0; i < segments.size(); i++)
            rot_out[i] = segments[i]->get_clamped_rotation(false);

        // solve base rotation
        auto seg_6 = s6->get_origin(false);
        auto seg2d_6 = glm::vec2(seg_6.x, seg_6.z);

        auto dif_6 = target_2d - seg2d_6;
        auto atan2_6 = atan2(dif_6[1], dif_6[0]);
        auto norm_6 = atan2_6 / M_PI;
        auto rot_6 = (norm_6 + 1.0f) / 2.0f;
        auto deg_6 = rot_6 * 360.0f;
        auto serv_6 = rot_6;

        glm::vec3 seg_5 = s5->get_origin(false);
        glm::vec3 seg_5_real = glm::vec3(seg_5.x, seg_5.z, seg_5.y);
        glm::vec3 target_for_calc = target_coords;

        auto target_pl3d = util::map_to_xy<float>(target_for_calc, deg_6, glm::vec3(y_axis), seg_5);
        auto target_pl2d = glm::vec2(target_pl3d.x, target_pl3d.y);
        auto plo3d = glm::vec3(0.0f);
        auto plo2d = glm::vec2(0.0f);
        //auto pl3d = glm::vec3(500.0f, 0.0f, 0.0f);
        //auto pl2d = glm::vec2(pl3d);
        //auto s2d = glm::vec2(500);

        std::vector<segment_t*> remaining_segments;
        remaining_segments.assign(segments.begin() + 2, segments.end());

        glm::vec2 prev_origin = target_pl2d;
        std::vector<glm::vec2> new_origins;
 
        if (debug_pedantic)
            printf("target_pl2d <%.2f,%.2f> target_pl3d <%.2f,%.2f,%.2f> target_real <%.2f,%.2f,%.2f> seg_5 <%.2f,%.2f,%.2f> deg_6: %.2f\n", target_pl2d.x, target_pl2d.y, target_pl3d.x, target_pl3d.y, target_pl3d.z, target_coords.x, target_coords.y, target_coords.z, seg_5.x, seg_5.y, seg_5.z, deg_6);

        while (true) {
            if (remaining_segments.size() < 1) {
                if (debug_pedantic)
                    puts("No more segments");
                break;
            }

            auto seg = remaining_segments.back();
            float segment_radius = seg->get_length();
            float total_length = 0.0f;

            for (auto *x : remaining_segments)
                total_length += x->get_length();

            float dist_to_segment = total_length - segment_radius;
            float segment_min = total_length - (segment_radius * 2);
            float dist_origin_to_prev = glm::distance<2, float>(plo2d, prev_origin);
            glm::vec2 mag = glm::normalize(prev_origin - plo2d);
            
            seg->debug_color = {0.,1.,0};
            bool skip_optim = false;

            if (remaining_segments.size() < 3) {
                if (debug_pedantic)
                    puts("2 or less segments left");
                skip_optim = true;
            }

            float equal_mp = ((dist_origin_to_prev * dist_origin_to_prev) -
                        (segment_radius * segment_radius) +
                        (dist_to_segment * dist_to_segment)) /
                        (2 * dist_origin_to_prev);

            float rem_dist = dist_origin_to_prev - equal_mp;
            float rem_min = -segment_radius/2.0f;
            float rem_retract = 0.0f;
            float rem_extend = segment_radius * 0.5f;
            float rem_ex2 = rem_extend * 1.75f;
            float rem_max = segment_radius * 0.95f;

            if (debug_pedantic)
                printf("servo: %i, rem_dist: %.2f, rem_max: %.2f, equal_mp: %.2f, segment_radius: %.2f, dist_origin_to_prev: %.2f, dist_to_segment: %.2f, total_length: %.2f, prev_origin <%.2f,%.2f>\n", seg->servo_num, rem_dist, rem_max, equal_mp, segment_radius, dist_origin_to_prev, dist_to_segment, total_length, prev_origin.x, prev_origin.y);

            auto new_origin = prev_origin;

            if (dist_to_segment < 0.05f) {
                new_origins.push_back(new_origin);
                if (debug_pedantic)
                    puts("Convergence");
                break;
            }

            if (rem_dist > rem_max) {
                if (debug_pedantic) {
                    puts("Not enough overlap");
                    printf("rem_dist: %.2f rem_max: %.2f\n", rem_dist, rem_max);
                    seg->debug_color = {0.,0.,0.};
                }
            }

            if (!skip_optim) {
                if (segment_radius > dist_origin_to_prev) {
                    auto v = rem_extend - (segment_radius - dist_origin_to_prev);
                    equal_mp = dist_origin_to_prev - v;
                    if (debug_pedantic) {
                        seg->debug_color = {1.,0,0};
                        puts("Too close to origin");
                    }
                } else
                if (rem_dist < rem_extend && total_length > dist_origin_to_prev) {
                    equal_mp = dist_origin_to_prev - rem_extend;
                    if (debug_pedantic) {
                        seg->debug_color = {1.,1,1};
                        puts("Maintain center of gravity");
                    }
                } else
                if (rem_dist < rem_retract && total_length > dist_origin_to_prev) {
                    equal_mp = dist_origin_to_prev - rem_retract;
                    if (debug_pedantic) {
                        puts("Too much leftover length");
                        seg->debug_color = {1.,.5,.5};
                    }
                } else
                if (rem_dist < rem_ex2 && rem_dist >= rem_extend && total_length > dist_origin_to_prev) {
                    if (debug_pedantic) {
                        puts("Too much leftover length");
                        seg->debug_color = {0.,.5,.5};
                    }
                    auto r = rem_ex2 - rem_extend;
                    r = (rem_dist - rem_extend) / r;
                    auto v = r / 2.0f;

                    if (v > 0.4f)
                        v -= (v - 0.38f);

                    equal_mp = dist_origin_to_prev - (v * segment_radius + rem_extend);
                }

                if (rem_dist < rem_min) {
                    if (debug_pedantic)
                        puts("Too much overlap");
                        seg->debug_color = {0.25,0.25,0.25};
                    rem_dist = rem_min;
                } else
                if (rem_dist > rem_max) {
                    if (debug_pedantic)
                        puts("Not enough overlap");
                        seg->debug_color = {0,0,0.};
                } else {
                    rem_dist = dist_origin_to_prev - equal_mp;
                }
            }

            if (debug_pedantic)
                printf("rem_dist: %.2f, rem_max: %.2f, equal_mp: %.2f, dist_origin_to_prev: %.2f, dist_to_segment: %.2f\n", rem_dist, rem_max, equal_mp, dist_origin_to_prev, dist_to_segment);

            auto mp_vec = mag * equal_mp;
            //auto n = sqrtf((segment_radius - rem_dist) * (segment_radius - rem_dist));
            auto n = sqrtf(abs((segment_radius * segment_radius) - (rem_dist * rem_dist)));
            auto o = atan2(mag.y, mag.x) - (M_PI / 2.0f);
            auto new_mag = glm::normalize(glm::vec2(cosf(o),sinf(o)));
            new_origin = mp_vec + (new_mag * n);
            auto new_origin3d = glm::vec3(new_origin.x, new_origin.y, 0.0f);
            auto dist_new_prev = glm::distance<2, float>(new_origin, prev_origin);

            if (debug_pedantic)
                printf("mp_vec <%.2f %.2f>, segment_radius: %.2f, rem_dist: %.2f, n: %.2f, o: %.2f, new_mag <%.2f,%.2f>, dist_new_prev: %.2f\n", mp_vec[0], mp_vec[1], segment_radius, rem_dist, n, o, new_mag[0], new_mag[1], dist_new_prev);

            if (calculation_failure)
                seg->debug_color = {1.0,0,0};

            float tolerable_distance = 10.0f;

            if (abs(dist_new_prev - segment_radius) > tolerable_distance) {
                if (debug_pedantic)
                    puts("Distance to prev is too different");
                calculation_failure = true;
            }

            if (remaining_segments.size() < 1 && glm::distance<2, float>(new_origin, target_pl2d) > tolerable_distance) {
                if (debug_pedantic)
                    puts("Distance to target is too far");
                calculation_failure = true;
            }

            //if (isnot_real(new_origin.x) || isnot_real(new_origin.y))
            //    new_origin = prev_origin;//glm::vec2(0.0f);

            nocalc:;

            prev_origin = new_origin;
            new_origins.push_back(new_origin);
            remaining_segments.pop_back();
        }

        if (calculation_failure) {
            if (debug_pedantic)
                puts("Failed to calculate");
            return glfail;
        } else {
            if (new_origins.size() < 1) {
                if (debug_pedantic)
                    puts("Not enough origins");
                return glfail;
            }

            float prevrot = 0.0f;
            auto prevmag = glm::vec2(0.0f,1.0f);
            auto prev = glm::vec2(0.0f);
            new_origins.pop_back();
            std::reverse(new_origins.begin(), new_origins.end());
            new_origins.push_back(target_pl2d);

            for (int i = 0; i < new_origins.size(); i++) {
                auto cur = new_origins[i];

                auto dif = cur - prev;
                auto mag = glm::normalize(dif);

                auto servo = segments[i + 2];
                auto calcmag = mag;

                auto rot = atan2(calcmag.x, calcmag.y) - prevrot;

                auto deg = ((glm::degrees(rot)) / 180.0f) * 0.5f + 1.0f;
                deg *= 360;

                if (isnot_real(deg)) {
                    deg = rot_out[i + 2];
                    rot = glm::radians(deg);
                }
                
                if (debug_pedantic)
                    printf("servo: %i, rot: %.2f, deg: %.2f, prevrot: %.2f, calcmag[0]: %.2f, calcmag[1]: %.2f, cur[0]: %.2f, cur[1]: %.2f, prev[0]: %.2f, prev[1]: %.2f\n", servo->servo_num, rot, deg, prevrot, calcmag.x, calcmag.y, cur.x, cur.y, prev.x, prev.y);

                
                rot_out[i + 2] = deg;

                prev = cur;
                prevmag = mag;
                prevrot = prevrot + rot;
            }
        }

        if (debug_pedantic)
            printf("Initial rot: %.2f,%.2f,%.2f,%.2f,%.2f\n", rot_out[0], rot_out[1], rot_out[2], rot_out[3], rot_out[4]);

        rot_out[1] = serv_6 * 360;

        if (debug_pedantic)
            printf("End rot: %.2f,%.2f,%.2f,%.2f,%.2f\n", rot_out[0], rot_out[1], rot_out[2], rot_out[3], rot_out[4]);
        for (int i = 0; i < segments.size(); i++) {
            auto wrapped = util::wrap(rot_out[i], -180, 180);
            segments[i]->set_rotation_bound(wrapped);
        }

        set_sliders_from_segments();

        return glsuccess;
    }
};

struct joystick_t {
    struct joystick_device_t {
        joystick_device_t(int jid):joystick_device_t(get_device(jid).second) {}
        joystick_device_t() {}
        
        int axis_count, button_count, jid;
        std::string guid, name, gp_name;
        std::map<int, bool> held_buttons;
        std::vector<float> deadzones;
        GLFWgamepadstate state;

        // call keys to update their state and get the updated state, stash output if need be, calling twice will ruin press detection
        int get_button(int button) {
            if (state.buttons[button]) {
                if (held_buttons[button]) {
                    return GLFW_REPEAT;
                }
                held_buttons[button] = 1;
                return GLFW_PRESS;
            }
            held_buttons[button] = 0;
            return 0;
        }

        void set_deadzones() {
            assert(deadzones.size() == axis_count && "Deadzone array length and axis_count\n");
            memcpy(deadzones.data(), &state.axes, axis_count * sizeof deadzones[0]);
        }

        void update() {
            if (!glfwGetGamepadState(jid, &state)) {
                const float *axes = glfwGetJoystickAxes(jid, &axis_count);
                const unsigned char *buttons = glfwGetJoystickButtons(jid, &button_count);

                if (axis_count < 6 || axis_count > 6) {
                    // redundant message, axis/button count available in the device object
                    //std::string msg = std::format("{} axes required (found {})", 6, jd.axis_count);
                    //jd.gp_name = msg;
                    if (axis_count < 6)
                        return;
                    // copy count based off GLFWgamepadstate
                    //jd.axis_count = 6;
                }

                memcpy(&state.axes, axes, sizeof state.axes);
                memcpy(&state.buttons, buttons, sizeof state.buttons);

                // remap for the controller im using
                std::swap(state.axes[2], state.axes[4]);
                std::swap(state.axes[3], state.axes[2]);
            }
        }

        static std::pair<std::string, joystick_device_t> get_device(int jid) {
            std::pair<std::string, joystick_device_t> kv;

            auto &jd = kv.second;
            jd.jid = jid;
            const char *guid = glfwGetJoystickGUID(jid);
            const char *gp_name = glfwGetGamepadName(jid);
            const char *name = glfwGetJoystickName(jid);

            assert(guid && "GUID null\n");

            if (!guid) 
                jd.guid = std::to_string((size_t)glfwGetJoystickUserPointer(jid));
            else
                jd.guid = guid;
            if (!gp_name) 
                jd.gp_name = "Generic";
            else
                jd.gp_name = gp_name;
            if (!name) 
                jd.name = "Joystick";
            else
                jd.name = name;

            kv.first = jd.guid;

            glfwGetJoystickAxes(jid, &jd.axis_count);
            glfwGetJoystickButtons(jid, &jd.button_count);
            
            jd.deadzones.assign(jd.axis_count, 0.0f);

            return kv;
        }
    };

    using map_type = std::map<std::string, joystick_device_t>;

    map_type joysticks;
    
    bool pedantic_debug = false;
    bool camera_move = false;

    std::mutex no_reentrancy;

    std::map<int, std::string> button_mapping = {
        {GLFW_GAMEPAD_BUTTON_GUIDE, "Guide"},
        {GLFW_GAMEPAD_BUTTON_LEFT_BUMPER, "L1"},
        {GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER, "R1"},
        {GLFW_GAMEPAD_BUTTON_B, "B"},
        {GLFW_GAMEPAD_BUTTON_A, "A"},
        {GLFW_GAMEPAD_BUTTON_Y, "Y"},
        {GLFW_GAMEPAD_BUTTON_X, "X"},
        {GLFW_GAMEPAD_BUTTON_START, "Start"},
        {GLFW_GAMEPAD_BUTTON_CIRCLE, "Circle"}
    };

    std::map<int, std::string> axis_mapping = {
        {GLFW_GAMEPAD_AXIS_LEFT_X, "Left X"},
        {GLFW_GAMEPAD_AXIS_LEFT_Y, "Left Y"},
        {GLFW_GAMEPAD_AXIS_RIGHT_X, "Right X"},
        {GLFW_GAMEPAD_AXIS_RIGHT_Y, "Right Y"},
        {GLFW_GAMEPAD_AXIS_LEFT_TRIGGER, "Left Trig"},
        {GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER, "Right Trig"}
    };

    template<typename T, int c = 3>
    glm::vec<c, T> scale_axes(glm::vec<c, T> &in) {
        glm::vec<c, T> ret(0.0);
        for (int i = 0; i < c; i++) {
            T sign = in[i] >= 0 ? 1 : -1;
            ret[i] = (0.0 + (1.0 * powf64(0.0 + abs(in[i]), 2))) * sign;
        }
        return ret;
    }

    void set_robot(double deltaTime) {
        for (auto &joy : joysticks) {
            auto &guid = joy.first;
            auto &jd = joy.second;
            auto &gp = jd.state;
            auto &axes = gp.axes;
            auto &buttons = gp.buttons;
            float tolerance = 0.12;

            auto _set_v3 = [&](vec3_d &a, vec3_d &b, int i, double t) {
                if (abs(b[i]) > t && abs(b[i]) < 100) {
                    a[i] += b[i];
                    return true;
                }
                return false;
            };

            auto set_v3 = [&](vec3_d &a, vec3_d &b, double t) {
                return _set_v3(a, b, 0, t) | 
                       _set_v3(a, b, 1, t) | 
                       _set_v3(a, b, 2, t);
            };

            if (camera_move) {
                float pitch = -axes[3];
                float yaw = axes[2];
                if (abs(pitch) <= tolerance)
                    pitch = 0;
                if (abs(yaw) <= tolerance)
                    yaw = 0;
                camera->joystick_move(window, pitch * 2 * deltaTime, yaw * 2 * deltaTime);


                vec3_d jd(-axes[1], 0, axes[0]);

                //jd = scale_axes(jd);

                vec3_d ndz(0.0);
                set_v3(ndz, jd, tolerance);
                ndz = scale_axes(ndz);
                if (buttons[GLFW_GAMEPAD_BUTTON_A])
                    ndz.y += 0.5;//jd.y += 0.5;
                if (buttons[GLFW_GAMEPAD_BUTTON_B])
                    ndz.y -= 0.5;//jd.y -= 0.5;
                auto cyw = glm::radians(camera->yaw);
                auto vvv = vec3_d(
                    (cos(cyw) * ndz.x) - (sin(cyw) * ndz.z),
                    ndz.y,
                    (sin(cyw) * ndz.x) + (cos(cyw) * ndz.z)
                );
                
                camera->position += vvv * deltaTime;
            } else {
                vec3_d jd(-axes[1], -axes[3], axes[0]);
                //jd *= 0.25f;
                vec3_d ndz(0.0);
                set_v3(ndz, jd, tolerance);

                ndz = scale_axes(ndz);
                ndz *= 0.25;
                auto cyw = glm::radians<double>(camera->yaw);
                auto vvv = vec3_d(
                    (cos(cyw) * ndz.x) - (sin(cyw) * ndz.z),
                    ndz.y,
                    (sin(cyw) * ndz.x) + (cos(cyw) * ndz.z)
                ) * deltaTime;
                robot_target += vvv;

                if (glm::length2(vvv) > 0)
                    kinematics->solve_inverse(robot_target);

                vec3_d pd(axes[2] * 1.5, (axes[4] + 1) / 2, (axes[5] + 1) / 2), ndd(0.0);
                set_v3(ndd, pd, tolerance);
                ndd = scale_axes(ndd) * deltaTime;

                if (glm::length2(ndd) > 0) {
                    float s2r = s2->get_rotation(false);
                    float s1r = s1->get_rotation(false);

                    s2r += ndd.x * deltaTime;
                    s1r -= ndd.y * deltaTime;
                    s1r += ndd.z * deltaTime;

                    s2r = util::clip(s2r, -120, 120); // because we can't check robot_interface unless refactor, works for now
                    s1r = util::clip(s1r, -50, 50);

                    s2->set_rotation(s2r);
                    s1->set_rotation(s1r);
                }
            }
        }
    }

    void rest_robot();

    void rest_position_robot();

    void query_robot();

    void connect_robot();

    void process_input(double deltaTime) {
        for (auto &joy : joysticks) {
            auto &jd = joy.second;
            auto &gp = jd.state;
            auto dz = jd.deadzones.data();
            int dp[] = {jd.get_button(GLFW_GAMEPAD_BUTTON_DPAD_UP), 
                        jd.get_button(GLFW_GAMEPAD_BUTTON_DPAD_DOWN), 
                        jd.get_button(GLFW_GAMEPAD_BUTTON_DPAD_LEFT), 
                        jd.get_button(GLFW_GAMEPAD_BUTTON_DPAD_RIGHT)};

            int bump[] = {jd.get_button(GLFW_GAMEPAD_BUTTON_LEFT_BUMPER),
                          jd.get_button(GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER)};

            if (jd.get_button(GLFW_GAMEPAD_BUTTON_GUIDE))
                jd.set_deadzones();

            if (bump[0] + bump[1] == 3)
                pedantic_debug = !pedantic_debug;

            if (jd.get_button(GLFW_GAMEPAD_BUTTON_BACK) == GLFW_PRESS)
                camera_move = !camera_move;

            if (jd.get_button(GLFW_GAMEPAD_BUTTON_START) == GLFW_PRESS)
                ::reset();

            if (!camera_move && jd.get_button(GLFW_GAMEPAD_BUTTON_A) == GLFW_PRESS)
                rest_robot();

            if (!camera_move && (dp[0] || dp[1])) {
                auto dir = s3->get_segment_vector();
                auto sp = vec3_d(dir) * vec3_d(0.01) * deltaTime;
                if (dp[0])
                    robot_target -= sp;
                if (dp[1])
                    robot_target += sp;
                kinematics->solve_inverse(robot_target);
            }

            if (!camera_move && (dp[2] || dp[3]) && bump[0] == 1)
                rest_position_robot();

            if (camera_move && dp[0] == 1)
                connect_robot();

            if (camera_move && dp[1] == 1)
                toggle_fullscreen_state();

            if (camera_move && dp[2] == 1)
                viewport_inversion = glm::scale(viewport_inversion, {-1,-1,1});

            if (jd.get_button(GLFW_GAMEPAD_BUTTON_Y) == GLFW_PRESS) {
                debug_mode = !debug_mode;
                debugInfo->hidden = !debug_mode;
                debugToggle->modified = true;
                debugToggle->toggle_state = debug_mode;
            }

            if (jd.get_button(GLFW_GAMEPAD_BUTTON_X) == GLFW_PRESS)
                query_robot();

            for (int i = 0; i < jd.axis_count; i++)
                gp.axes[i] -= dz[i];
        }
    }

    void update(double deltaTime) {
        GLFWgamepadstate p;
        glfwGetGamepadState(0, &p);

        if (!no_reentrancy.try_lock())
            return;

        for (auto &joy : joysticks)
            joy.second.update();

        process_input(deltaTime);
        set_robot(deltaTime);

        no_reentrancy.unlock();
    }

    std::string debug_info() {
        std::string info;
        for (auto &joy : joysticks) {
            auto &jid = joy.first;
            auto &jd = joy.second;
            info += std::format("Joystick: {}\n  Axes: {}\n  Buttons: {}\n  Jid: {}\n", jd.gp_name, jd.axis_count, jd.button_count, jd.jid);
            auto &gp = jd.state;
            for (int i = 0; i < sizeof gp.axes / sizeof gp.axes[0]; i++) {
                info += std::format("  {}: {}\n", axis_mapping[i], gp.axes[i]);
            }
            auto &jh = jd.held_buttons;
            for (int i = 0; i < sizeof gp.buttons / sizeof gp.buttons[0]; i++) {
                if (button_mapping.contains(i))
                    info += std::format("  {}: {} {}\n", button_mapping[i], gp.buttons[i], jh[i]);
                else
                if (pedantic_debug)
                    info += std::format("  {}: {} {}\n", i, gp.buttons[i], jh[i]);
            }
            /*
            const auto *buttons = glfwGetJoystickButtons(jd.jid, &jd.button_count);
            for (int i = 0; i < jd.button_count; i++)
                info += std::format("+{}: {}{}", i, buttons[i], (i+1) % 4 == 0 ? '\n' : ' ');
            info += "\n";
            */
        }   
        return info;
    }

    void remove(const std::string &guid) {
        fprintf(stderr, "Remove joystick %s\n", guid.c_str());
        joysticks.erase(guid);
    }

    void reset() {

    }

    void query_joysticks() {
        map_type joys = map_type();
        static bool first_run = true;

        for (int jid = GLFW_JOYSTICK_1; jid < GLFW_JOYSTICK_LAST; jid++) {
            if (!glfwJoystickPresent(jid))
                continue;

            // Note, GUID can match two discrete joysticks from a single device

            auto kv = joystick_device_t::get_device(jid);
            auto &jd = kv.second;

            if (joysticks.contains(kv.first)) {
                //joys.insert({kv.first, joysticks[kv.first]});
                using it = map_type::iterator;
                using mv = std::move_iterator<it>;
                it p = joysticks.find(kv.first);
                joys.insert(mv(p),mv(p));
                continue;
            }

            if (kv.second.axis_count != 6) {
                if (first_run && debug_mode)
                    fprintf(stderr, "6 axes joystick device required, device %i \"%s\" \"%s\" \"%s\"\n", jid, jd.gp_name.c_str(), jd.name.c_str(), jd.guid.c_str());
                continue;
            }

            fprintf(stderr, "Add joystick %i \"%s\" \"%s\" \"%s\"\n", jid, jd.gp_name.c_str(), jd.name.c_str(), jd.guid.c_str());
            jd.update();
            jd.set_deadzones();
            joys.insert(kv);
        }

        for (auto &jd : joysticks) 
            if (!joys.contains(jd.first))
                fprintf(stderr, "Remove joystick %i \"%s\" \"%s\" \"%s\"\n", jd.second.jid, jd.second.gp_name.c_str(), jd.second.name.c_str(), jd.second.guid.c_str());

        first_run = false;
        joysticks = std::move(joys);
        update(0.1);
    }
};

struct robot_interface_t {
    hid_device *handle;
    std::string serial_number;
    bool servo_sleep_on_destroy = true;
    bool virtual_output = false;

    using clk = std::chrono::high_resolution_clock;
    using tp = std::chrono::time_point<clk>;
    using dur = std::chrono::duration<long, std::milli>;

    template<typename RB, 
             typename PERIOD_T = int,
             typename SERVO_T = RB::servo_type,
             typename VT_T = RB::value_type,
             typename RB_TP = RB::tp,
             typename PAIR_T = std::pair<SERVO_T,SERVO_T>>
        requires std::is_base_of_v<robot_servo_T<SERVO_T, VT_T>, RB>
    bool get_servo_command(RB *servo, PERIOD_T &r_period, RB_TP &batch_time, PAIR_T &cmd) {
        auto targetf = servo->get_clamped_rotation();
        auto targeti = servo->get_servo(targetf);//servo->to_servo(targetf);
        auto servo_min = servo->servo_min;
        auto servo_max = servo->servo_max;
        auto initialp = servo->get_servo_start();
        auto initialpf = servo->get_servo_degrees(initialp);
        auto intrp = servo->get_servo_interpolated();

        if (targeti < servo_min || targeti > servo_max)
            targeti = util::clip(targeti, servo_min, servo_max);

        auto dist = targeti - intrp;

        servo->last_command = batch_time;

        if (abs(dist) < servo->min_command_threshold && abs(dist) < 1) {
            servo->servo_end_position = targeti;
            servo->servo_cur_position = targeti;
            return false;
        }

        auto mv = (r_period/1000.0) * servo->degrees_per_second;
        auto mvdist = servo->servo_cur_position - intrp;
        auto accel = mvdist / mv;
        auto jerk = (mvdist * mv) / mv;
        auto rintrp = intrp;

        if (abs(jerk) > mv / 2 && abs(mvdist) > 0) {
            intrp += (mvdist * .5);
        } else
        if (abs(dist) < mv / 2 && abs(jerk) < mv / 4) {
            intrp = targeti;
            if (debug_pedantic)
                fprintf(stderr, "Force set targeti\n");
        }

        servo->servo_end_position = targeti;
        servo->servo_cur_position = intrp;

        if (debug_pedantic)
            fprintf(stderr, "Send constant time s%i (%i/initialp -> %i/rintrp (jerk comp %i/intrp)) (%i/mvdist) (%i/targeti %.2f/targetf : %.2f/initialpf) = %i/dist (%i r_period/ms %.2lf mv/intpersec) accel %.2f jerk %.2f\n", servo->servo_num, initialp, rintrp, intrp, mvdist, targeti, targetf, initialpf, dist, r_period, mv, accel, jerk);

        cmd = { servo->servo_num, intrp };

        return true;
    }

    void update() {
        if (!handle && !virtual_output)
            return;

        using sv_t = segment_t::servo_type;
        using tp_t = segment_t::tp;
        using clk_t = segment_t::clk;
        using pair_t = std::pair<sv_t, sv_t>;

        static tp_t last_batch = clk_t::now();
        tp now_batch = clk_t::now();

        static bool constant_speed = false;
        static int u_period = 10;
        static int m_period = 200;
        static int t_overlap = 0;
        int r_period = int(std::chrono::duration_cast<dur>(now_batch - last_batch).count()) + t_overlap;

        if (!constant_speed && r_period < u_period)
            return;

        if (!constant_speed && r_period > m_period)
            r_period = m_period;

        std::vector<pair_t> cmds;

        for (auto *sv : servo_segments) {
            pair_t p;
            if (get_servo_command(sv, r_period, now_batch, p))
                cmds.push_back(p);
        }

        if (cmds.size() > 0) {
            set_servos(cmds, r_period);
            last_batch = now_batch;
        }
    }

    robot_interface_t(bool permit_virtual = false):virtual_output(permit_virtual) {
        init();
    }

    ~robot_interface_t() {
        destroy();
    }

    void destroy() {
        if (handle) {
            if (servo_sleep_on_destroy)                
                servos_off();
            fprintf(stderr, "Close robot connection\n");
            close();
        }
        hid_exit();
    }

    std::string get_hid_error() {
        std::wstring err = hid_error(0);
        return std::string(err.begin(), err.end());
    }

    void read_all(bool set_pos = false) {
        if (!handle)
            return;

        const unsigned char cmd[11] = {
            0x55, 0x55, 9, 21, 6, 1, 2, 3, 4, 5, 6
        };
        hid_write(handle, &cmd[0], 11);

        unsigned char ret[100];
        int count = hid_read_timeout(handle, &ret[0], 100, 2000);
        if (count < 6) {
            if (debug_mode)
                fprintf(stderr, "Failed to read any bytes\n%s\n", get_hid_error().c_str());
            return;
        }

        count = ret[4];

        auto now_time = segment_t::clk::now();
        for (int i = 0; i < count; i++) {
            auto *seg = servo_segments[i];
            int index = 5 + 3 * i;
            int id = ret[index];
            int pos = (ret[index + 2] << 8) | ret[index + 1];
            unsigned short upos = (unsigned short)pos;
            seg->servo_cur_position = upos;
            if (set_pos)
                seg->servo_end_position = upos;
            seg->last_command = now_time;
            if (debug_pedantic)
                fprintf(stderr, "s%i r%i p%i\n", id, seg->servo_cur_position, seg->servo_end_position);
        }
    }

    //set no check
    void set_servo(int id, int position, int millis = 1000) {
        if (!handle)
            return;
            
        set_servos({{id,position}}, millis);
    }

    //set no check
    void set_servos(const std::vector<std::pair<int,int>> &poses, const int time = 1000) {
        if (!handle)
            return;

        const int count = poses.size() * 3 + 7;
        assert(poses.size() > 0 && "No poses");
        assert(poses.size() < 255 && count < 255 && "Should not be that big\n");
        unsigned char cmd[count];
        unsigned char header[7] = {0x55, 0x55, (unsigned char)(count - 2), 0x03, (unsigned char)(poses.size()),
            (unsigned char)(time & 0xFF), (unsigned char)(time >> 8)
        };
        memcpy(&cmd[0], &header[0], 7);

        int i = 0;
        for (auto &sv : poses) {
            int offset = i++ * 3 + 7;
            cmd[offset] = sv.first;
            cmd[offset+1] = sv.second & 0xFF;
            cmd[offset+2] = sv.second >> 8;
        }

        hid_write(handle, &cmd[0], count);
    }

    //set no check
    void servos_off() {
        if (!handle)
            return;

        unsigned char cmd[11] = { 0x55, 0x55, 9, 20, 6, 1, 2, 3, 4, 5, 6 };
        hid_write(handle, &cmd[0], 11);
        usleep(100000); //why doesn't the command work every time, im trying to fix it
    }

    void init() {
        hid_init();
        handle = nullptr;
    }

    void set_robot_defaults() {
        if (!handle && virtual_output)
            serial_number = "Virtual Robot";
    }

    int open(unsigned short vendor_id, unsigned short product_id, wchar_t *serial_number_w = nullptr) {
        if (handle)
            close();
        handle = hid_open(vendor_id, product_id, serial_number_w);

        if (!handle) {
            if (virtual_output) {
                fprintf(stderr, "No handle, create virtual output\n");
                set_robot_defaults();
                return glsuccess;
            }
            std::wstring err = hid_error(0);
            std::string rr(err.begin(), err.end());
            fprintf(stderr, "Not connected to robot, (nonfatal) reason: %s\n", rr.c_str());
            serial_number = "No USB";
            return glfail;
        }

        std::wstring wstr;
        if (!serial_number_w) {
            wchar_t wbuf[100];
            if (!hid_get_serial_number_string(handle, &wbuf[0], 100))
                wstr = std::wstring(&wbuf[0]);
            else 
                wstr = L"No serial";
        } else {
            wstr = std::wstring(serial_number_w);
        }
        serial_number = std::string(wstr.begin(), wstr.end());

        hid_set_nonblocking(handle, 0);
        fprintf(stderr, "Open robot connection\n");

        set_robot_defaults();
        read_all(true);
        set_segments_from_robot();

        return glsuccess;
    }

    void close() {
        if (!handle)
            return;
        hid_close(handle);
        handle = nullptr;
    }

    // Try to close and open connection, do not destroy
    void reset(unsigned short vendor_id, unsigned short product_id, wchar_t *serial_number_w = nullptr) {
        open(vendor_id, product_id, serial_number_w);
    }

    std::string debug_info() {
        std::string ret;
        ret += std::format("USB: {}\n", serial_number);
        for (auto *seg : servo_segments) {
            ret += std::format("  {}: {}\n", seg->servo_num, seg->servo_cur_position);
        }
        return ret;
    }
};

void joystick_t::query_robot() {
    robot_interface->read_all();
    for (auto *seg : servo_segments) {
        fprintf(stderr, "%i: %i (%.2f deg), ", seg->servo_num, seg->servo_cur_position, seg->to_degrees(seg->servo_cur_position));
    }
    fputs("\n", stderr);
}

void joystick_t::rest_robot() {
    robot_interface->servos_off();
}

void rest_position_blocking() {
    float iv = 1000.0;
    robot_interface->set_servos({{1,500},
                                 {2,500},
                                 {3,500},
                                 {4,500},
                                 {5,500},
                                 {6,500}}, iv);

    std::this_thread::sleep_for(dur(iv));

    robot_interface->set_servo(3, 1, iv);
}

void joystick_t::rest_position_robot() {
    std::thread(rest_position_blocking);
}

void joystick_t::connect_robot() {
    robot_interface->open(1155, 22352);
    set_segments_from_robot();
}

void set_segments_from_sliders() {
    for (int i = 0; i < servo_sliders.size() && i < servo_segments.size(); i++)
        servo_segments[i]->set_rotation_bound(servo_sliders[i]->value);

    robot_target = s3->get_origin(false) + s3->get_segment_vector(false);
}

void servo_slider_update(ui_slider_t* ui, ui_slider_t::ui_slider_v value) {
    set_segments_from_sliders();
}

void update_whatever(ui_slider_t* ui, ui_slider_t::ui_slider_v value) {
    for (int i = 0; i < slider_whatever.size(); i++) {
        float value = slider_whatever.at(i)->value;
        if (debug_pedantic)
            printf("%.4f%c",value, i == slider_whatever.size()-1?'\n':',');

        auto &gbl = global_text_parameters;

        switch (i) {
            case 0: gbl.scrCharacterTrimX = value; break;
            case 1: gbl.scrCharacterTrimY = value; break;
            case 2: gbl.texCharacterTrimX = value; break;
            case 3: gbl.texCharacterTrimY = value; break;
            case 4: gbl.scrCharacterSpacingScaleX = value; break;
            case 5: gbl.scrCharacterSpacingScaleY = value; break;
            case 6: gbl.scrScaleX = value; break;
            case 7: gbl.scrScaleY = value; break;
            case 8: title_height = value; break;
            case 9: value_subpos_x = value; break;
            case 10: value_subpos_y = value; break;
            case 11: camera->yaw = value; break;
            case 12: precise_factor = value; break;
            case 13: slider_frac_w = value; break;
            case 14: slider_frac_h = value; break;
            case 15: textProgram->mixFactor = value; break;
        }
    }

    ui_servo_sliders->reset();
}

void set_sliders_from_segments() {
    for (int i = 0; i < servo_sliders.size() && i < servo_segments.size(); i++)
        servo_sliders[i]->set_value(servo_segments[i]->get_clamped_rotation(), false);
}

void set_segments_from_robot() {
    for (int i = 0; i < servo_sliders.size() && i < servo_segments.size(); i++) {
        auto *sg = servo_segments[i];
        auto *ui = servo_sliders[i];
        sg->set_rotation(sg->get_servo_interpolated_degrees());
        ui->set_value(sg->get_clamped_rotation(), false);
        if (debug_pedantic)
            fprintf(stderr, "%i -> %s (%f -> %f)\n", sg->servo_num, ui->title_cached.c_str(), sg->get_rotation(false), sg->get_clamped_rotation());
    }

    robot_target = s3->get_origin(false) + s3->get_segment_vector(false);
}

void set_robot_from_segments() {
    for (int i = 0; i < servo_sliders.size(); i++) {
        auto *sg = servo_segments[i];
        sg->set_servo(sg->get_servo_interpolated());
    }
}

std::string segment_debug_info() {
    std::string ret = "";
    ret += std::format("{:>7} {: >12s} {: >13s}\n", "Servos:", "Interpolated", "Immediate");

    auto get_segment = [&](segment_t *seg) {
        return std::format("{:>3}: {:>9.2f} {:>5} {:>7.2f} {:>5}\n", seg->servo_num, seg->get_servo_interpolated_degrees(), seg->get_servo_interpolated(), seg->get_servo_degrees(), seg->get_servo());
    };

    for (auto *seg : servo_segments)
        ret += get_segment(seg);

    return ret;
}

void update_debug_info() {
    {
        const int bufsize = 1000;
        char char_buf[bufsize];

        glm::vec3 s3_t = s3->get_segment_vector() + s3->get_origin();
        
        debug_objects->add_sphere(robot_target, s3->model_scale);

        snprintf(char_buf, bufsize, 
        "%.0lf FPS %.2lf ms\nCamera %.2f %.2f %.2f\nFacing %.2f %.2f\nTarget %lf %lf %lf\ns3 %.2f %.2f %.2f\n%s%s%s",
        frametime.get_fps(), frametime.get_ms(), 
        camera->position.x, camera->position.y, camera->position.z,
        camera->yaw,camera->pitch,
        robot_target.x, robot_target.y, robot_target.z,
        s3_t.x,s3_t.y,s3_t.z,
        joysticks->debug_info().c_str(),
        segment_debug_info().c_str(),
        robot_interface->debug_info().c_str()
        );
        debugInfo->set_string(&char_buf[0]);
    }
}

void outputTest() {
   glBegin(GL_TRIANGLES);
        glColor3f(1.0f, 0.0f, 0.0f); // Red
        glVertex3f(-0.6f, -0.4f, 0.0f);
        glColor3f(0.0f, 1.0f, 0.0f); // Green
        glVertex3f(0.6f, -0.4f, 0.0f);
        glColor3f(0.0f, 0.0f, 1.0f); // Blue
        glVertex3f(0.0f, 0.6f, 0.0f);
    glEnd();
}

int init_context() {
    if (!glfwInit())
        handle_error("Failed to initialize GLFW");

    initial_window = {0,0,1600,900};

    window = glfwCreateWindow(initial_window[2], initial_window[3], "xArm", nullptr, nullptr);

    if (!window)
        handle_error("Failed to create GLFW window");

    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetJoystickCallback(joystick_callback);
    glfwSwapInterval(1);

    glfwGetWindowPos(window, &initial_window.x, &initial_window.y);
    glfwGetWindowSize(window, &initial_window[2], &initial_window[3]);

    current_window = initial_window;

    signal(SIGQUIT, handle_signal);
    signal(SIGKILL, handle_signal);
    signal(SIGSTOP, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);

    return glsuccess;
}

int init() {
    camera = new camera_t();
    mainVertexShader = new shader_t(GL_VERTEX_SHADER);
    mainFragmentShader = new shader_t(GL_FRAGMENT_SHADER);
    textVertexShader = new shader_t(GL_VERTEX_SHADER);
    textFragmentShader = new shader_t(GL_FRAGMENT_SHADER);

    mainProgram = new shader_materials_t(shader_program_t(mainVertexShader, mainFragmentShader));
    textProgram = new shader_text_t(shader_program_t(textVertexShader, textFragmentShader));

    mainTexture = new texture_t();
    textTexture = new texture_t();

    robotMaterial = new material_t(mainTexture,mainTexture,1.0f);

    uiHandler = new ui_element_t(window, {-1.0f,-1.0f,2.0f,2.0f});
    //uiHandler->add_child(new ui_text_t(window, {0.0,0.0,.1,.1}, "Hello World!"));
    debugInfo = uiHandler->add_child(new ui_text_t(window, textProgram, textTexture, {-1.0f,-1.0f,2.0f,2.0f}, "", update_debug_info));
    debug_objects = new debug_object_t();
    ui_servo_sliders = uiHandler->add_child(new ui_element_t(window, uiHandler->XYWH));

    glm::vec4 sliderPos = {0.45, -0.95,0.5,0.1};
    glm::vec4 sliderAdd = {0.0,0.2,0,0};
    glm::vec2 sMM = {-180, 180.};
    
    int sl = 0;
    slider6 = ui_servo_sliders->add_child(new ui_slider_t(window, textProgram, textTexture, sliderPos, sMM.x, sMM.y, sl++, "Servo 6", true, servo_slider_update));
    slider5 = ui_servo_sliders->add_child(new ui_slider_t(window, textProgram, textTexture, sliderPos + (sliderAdd * glm::vec4(sl++)), sMM.x, sMM.y, 0., "Servo 5", true, servo_slider_update));
    slider4 = ui_servo_sliders->add_child(new ui_slider_t(window, textProgram, textTexture, sliderPos + (sliderAdd * glm::vec4(sl++)), sMM.x, sMM.y, 0., "Servo 4", true, servo_slider_update));
    slider3 = ui_servo_sliders->add_child(new ui_slider_t(window, textProgram, textTexture, sliderPos + (sliderAdd * glm::vec4(sl++)), sMM.x, sMM.y, 0., "Servo 3", true, servo_slider_update));
    slider2 = ui_servo_sliders->add_child(new ui_slider_t(window, textProgram, textTexture, sliderPos + (sliderAdd * glm::vec4(sl++)), sMM.x, sMM.y, 0., "Servo 2", true, servo_slider_update));
    slider1 = ui_servo_sliders->add_child(new ui_slider_t(window, textProgram, textTexture, sliderPos + (sliderAdd * glm::vec4(sl++)), sMM.x, sMM.y, 0., "Servo 1", true, servo_slider_update));
    //debugInfo = new ui_text_t(window, {0.0,0.0,1.,1.}, "Hello world 2!");
    
    slider_ambient = debugInfo->add_child(new ui_slider_t(window, textProgram, textTexture, sliderPos + (sliderAdd * glm::vec4(sl++)), -2., 2., -0.3, "Ambient Light", false));
    slider_diffuse = debugInfo->add_child(new ui_slider_t(window, textProgram, textTexture, sliderPos + (sliderAdd * glm::vec4(sl++)), -2., 2., 1.5, "Diffuse Light", false));
    slider_specular = debugInfo->add_child(new ui_slider_t(window, textProgram, textTexture, sliderPos + (sliderAdd * glm::vec4(sl++)), -2., 2., -0.4, "Specular Light", false));
    slider_shininess = debugInfo->add_child(new ui_slider_t(window, textProgram, textTexture, sliderPos + (sliderAdd * glm::vec4(sl++)), -2., 2., 0.6, "Shininess", false));

    sliderPos = glm::vec4{-0.95,-.2,0,0} + glm::vec4{0,0,.25,0.1};
    sliderAdd = {0,.105,0,0};

    auto &gbl = global_text_parameters;
    float defaults[] = {
        gbl.scrCharacterTrimX, gbl.scrCharacterTrimY, 
        gbl.texCharacterTrimX, gbl.texCharacterTrimY, 
        gbl.scrCharacterSpacingScaleX, gbl.scrCharacterSpacingScaleY,
        gbl.scrScaleX, gbl.scrScaleX,
        title_height, value_subpos_x, value_subpos_y,
        camera->yaw, precise_factor,
        slider_frac_w, slider_frac_h,
        textProgram->mixFactor
    };

    bool extra_slider_hidden = false;
    for (int i = 0; i < (sizeof defaults / sizeof defaults[0]); i++) {
        int stepover = 8;
        if (i == stepover)
            sliderPos += glm::vec4(sliderPos[2]+0.1,0,0,0);
        slider_whatever.push_back(debugInfo->add_child(new ui_slider_t(window, textProgram, textTexture, sliderPos + (sliderAdd * glm::vec4(float(i % stepover))), defaults[i]-2., defaults[i]+2., defaults[i], "", false, update_whatever, false, extra_slider_hidden)));
    }

    auto toggle_pos = glm::vec4{-.975,.925,.05,.05};
    auto toggle_add = glm::vec4{.110, 0, 0, 0};

    debugInfo->hidden = !debug_mode;
    debugToggle = uiHandler->add_child(new ui_toggle_t(window, textProgram, textTexture, toggle_pos, "Debug", debug_mode, [](ui_toggle_t* ui, bool state){
        debug_mode = state;
        debugInfo->hidden = !debug_mode;
    }));
    resetToggle = debugInfo->add_child(new ui_toggle_t(window, textProgram, textTexture, toggle_pos += toggle_add, "Reset", false, [](ui_toggle_t *ui, bool state) {
        if (state)
            uiHandler->reset();
        reset();
    }));
    interpolatedToggle = debugInfo->add_child(new ui_toggle_t(window, textProgram, textTexture, toggle_pos += toggle_add, "Intrp", model_interpolation, [](ui_toggle_t *ui, bool state){
        model_interpolation = state;
    }));
    resetConnectionToggle = debugInfo->add_child(new ui_toggle_t(window, textProgram, textTexture, toggle_pos += toggle_add, "Conn", false, [](ui_toggle_t *ui, bool state) {
        if (state) {
            robot_interface->reset(1155, 22352);
            resetConnectionToggle->set_state(false);
        }
    }));
    pedanticToggle = debugInfo->add_child(new ui_toggle_t(window, textProgram, textTexture, toggle_pos += toggle_add, "Verb", debug_pedantic, [](ui_toggle_t* ui, bool state){
        debug_pedantic = state;
    }));

    for (int i = 0; i < 5; i++)
        meshes.push_back(new mesh_t);

    segment_t **s_pos[] = { &sBase, &s6, &s5, &s4, &s3, &s2, &s1 };

    for (int i = 0; i < 7; i++)
        segments.push_back(*s_pos[i] = new segment_t);

    visible_segments = std::vector<segment_t*>({sBase, s6, s5, s4, s3});
    servo_segments = std::vector<segment_t*>({s6, s5, s4, s3, s2, s1});
    servo_sliders = std::vector({slider6, slider5, slider4, slider3, slider2, slider1});
    kinematics = new kinematics_t();

    joysticks = new joystick_t;
    robot_interface = new robot_interface_t(true);

    return glsuccess;
}

void reset() {
    camera = new (camera)camera_t;
    camera->position = {-30,20,0.};
    camera->yaw = 0.001;
    camera->pitch = 0.001;
    camera->fov = 90;
    for (auto &seg : servo_segments)
        seg->set_rotation(0);
    set_sliders_from_segments();
    set_robot_from_segments();
    robot_target = s3->get_segment_vector(false) + s3->get_origin(false);
}

int load() {
    if (mainTexture->generate(glm::vec4(0.0f,0.0f,0.0f,0.0f)) ||
        textTexture->load("assets/text.png"))
        handle_error("Failed to load textures");

    if (mainVertexShader->load("shaders/vertex.glsl") ||
mainFragmentShader->load("shaders/fragment.glsl") ||
textVertexShader->load("shaders/text_vertex_shader.glsl") ||
textFragmentShader->load("shaders/text_fragment_shader.glsl"))
        handle_error("Failed to load shaders");

    if ((mainProgram->load() ||
        textProgram->load()))
        handle_error("Failed to compile shaders");

    /*
        Forward: -Y
        Up: Z
        UV, Triangulate, Normals, Modifiers
        Selection only

        Shade auto smooth 30*
        Merge by distance 0.00001
        Origin on pivot point
    */
    const char *mesh_locs[5] = {
        "assets/xarm-sbase.obj",
        "assets/xarm-s6.obj",
        "assets/xarm-s5.obj",
        "assets/xarm-s4.obj",
        "assets/xarm-s3.obj"
    };

    int dhome = 500;
    int dpos = 500;
    int drp = 500;
    float dconv = 240.0f / 1000.0f;

    segment_t::robot_servo_type servo_vals[7] = {
        {},
        { 6, 0, 1146, 482, dpos, drp, 700.0f, dconv },  // base
        { 5, 148, 882, 505, dpos, drp, 700.0f, dconv }, // 5
        { 4, 0, 1042, 502, dpos, drp, 700.0f, -dconv }, // 4 (inverted)
        { 3, 38, 1000, dhome, dpos, drp, 700.0f, dconv }, // 3
        { 2, 0, 925, dhome, dpos, drp, 700.0f, dconv }, // wrist
        { 1, 200, 850, dhome, dpos, drp, 641.0f, dconv } // gripper
    };

    segment_t segment_vals[7] = {
        {nullptr, meshes[0], servo_vals[0], z_axis, 46.19},
        {sBase, meshes[1], servo_vals[1], z_axis, 35.98},
        {s6, meshes[2], servo_vals[2], y_axis, 100.0},
        {s5, meshes[3], servo_vals[3], y_axis, 96.0},
        {s4, meshes[4], servo_vals[4], y_axis, 150.0},
        {nullptr, nullptr, servo_vals[5], z_axis, 0},
        {nullptr, nullptr, servo_vals[6], z_axis, 0}
    };

    for (int i = 0; i < sizeof mesh_locs / sizeof mesh_locs[0]; i++)
        if (meshes[i]->loadObj(mesh_locs[i]))
            handle_error((std::string("Failed to load model: ") + mesh_locs[i]).c_str());

    for (int i = 0; i < sizeof segment_vals / sizeof segment_vals[0]; i++)
        new (segments[i]) segment_t(segment_vals[i]);

    reset();
    uiHandler->load();
    //debugInfo->load();

    joysticks->query_joysticks();
    robot_interface->open(1155, 22352);
    set_segments_from_robot();
    if (debug_pedantic)
        fprintf(stderr, "robot_target: %lf %lf %lf\n", robot_target[0], robot_target[1], robot_target[2]);

    return glsuccess;
}

int main() {
    if (init_context() || init() || load())
        handle_error("Failed to load", glfail);

    while (!glfwWindowShouldClose(window)) {
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        frametime.update();
        auto delta_time = frametime.get_delta_time<double>();

        handle_keyboard(window, delta_time);
        joysticks->update(delta_time * 60.0);
        robot_interface->update();

        mainProgram->use();
        //mainProgram->set_camera(camera);

        glm::vec3 diffuse(slider_diffuse->value), specular(slider_specular->value), ambient(slider_ambient->value);

        robotMaterial->shininess = slider_shininess->value;

        mainProgram->set_v3("eyePos", camera->position);
        mainProgram->set_v3("light.position", glm::vec3(5.0f, 15.0f, 5.0f));
        mainProgram->set_v3("light.ambient", ambient);
        mainProgram->set_v3("light.diffuse", diffuse);
        mainProgram->set_v3("light.specular", specular);

        mainProgram->set_material(robotMaterial);

        render::render_segments(visible_segments, mainProgram, camera, model_interpolation);

        if (debug_mode)
            debug_objects->render();    
        
        if (uiHandler->render())
            break;

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    safe_exit(0);
}

void destroy() {
    glfwTerminate();

    if (robot_interface)
        robot_interface->destroy();
}

void hint_exit() {
    glfwSetWindowShouldClose(window, 1);
}

void safe_exit(int errcode) {
    destroy();
    exit(errcode);
}

void handle_error(const char *str, int errcode) {
    fprintf(stderr, "Error: %s\n", str);
    safe_exit(errcode);
}

void handle_signal(int sig) {
    fprintf(stderr, "Close signal caught\n");
    hint_exit();
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    current_window[2] = width;
    current_window[3] = height;
    glViewport(0, 0, width, height);
    uiHandler->onFramebuffer(width, height);
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
    if (uiHandler->onMouse(button, action, mods))
        return;

    camera->mousePress(window, button, action, mods);
}

void cursor_position_callback(GLFWwindow *window, double x, double y) {
    if (uiHandler->onCursor(x, y))
        return;

    camera->mouseMove(window, x, y);
}

void toggle_fullscreen_state() {
    static tp last_toggle = hrc::now();
    auto dur = hrc::now() - last_toggle;

    if (dur.count() > .5) {
        fullscreen = !fullscreen;

        GLFWmonitor *monitor = glfwGetWindowMonitor(window);
        
        if (!monitor)
            monitor = glfwGetPrimaryMonitor();

        const GLFWvidmode *mode = glfwGetVideoMode(monitor);

        assert(mode != nullptr && "glfwGetVideoMode returned null\n");

        if (fullscreen)
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        else
            glfwSetWindowMonitor(window, 0, initial_window[0], initial_window[1], initial_window[2], initial_window[3], mode->refreshRate);            

        glfwGetWindowPos(window, &current_window[0], &current_window[1]);
        glfwGetWindowSize(window, &current_window[2], &current_window[3]);

        last_toggle = hrc::now();
    }
}

void handle_keyboard(GLFWwindow* window, float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        hint_exit();
        return;
    }

    if (glfwGetKey(window, GLFW_KEY_F11))
        toggle_fullscreen_state();

    if (uiHandler->onKeyboard(deltaTime))
        return;

    static bool x_press = false;
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
        if (!x_press)
            joysticks->query_robot();
        x_press = true;
    } else {
        x_press = false;
    }

    static bool b_press = false;
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
        if (!b_press)
            robot_interface->servos_off();
        b_press = true;
    } else {
        b_press = false;
    }

    camera->keyboard(window, deltaTime);

    int raise[] = {GLFW_KEY_R, GLFW_KEY_T, GLFW_KEY_Y, GLFW_KEY_U};
    int lower[] = {GLFW_KEY_F, GLFW_KEY_G, GLFW_KEY_H, GLFW_KEY_J};
    segment_t *segments[] = {s6,s5,s4,s3};

    float movementFactor = movementSpeed;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        movementFactor = rapidSpeed;

    bool change_2 = false;

    for (int i = 0; i < 4; i++) {
        auto *seg = segments[i];
        auto rot = seg->get_rotation(false);
        if (glfwGetKey(window, raise[i]) == GLFW_PRESS) {
            rot += movementFactor * deltaTime;
            seg->set_rotation(rot);
            change_2 = true;
        }

        if (glfwGetKey(window, lower[i]) == GLFW_PRESS) {
            rot -= movementFactor * deltaTime;
            seg->set_rotation(rot);
            change_2 = true;
        }
    }

    if (change_2) {
        robot_target = s3->get_segment_vector() + s3->get_origin();
        set_sliders_from_segments();
    }

    int robot3d[] = {GLFW_KEY_O, GLFW_KEY_L, GLFW_KEY_K, GLFW_KEY_SEMICOLON, GLFW_KEY_I, GLFW_KEY_P};
    bool robot3d_o[6];
    bool change = false;

    for (int i = 0; i < 6; i++) {
        robot3d_o[i] = (glfwGetKey(window, robot3d[i]) == GLFW_PRESS);

        if (robot3d_o[i]) {
            change = true;

            switch (i) {
                case 0:
                    robot_target.x += movementFactor * deltaTime;
                    break;
                case 1:
                    robot_target.x -= movementFactor * deltaTime;
                    break;
                case 2:
                    robot_target.z -= movementFactor * deltaTime;
                    break;
                case 3:
                    robot_target.z += movementFactor * deltaTime;
                    break;
                case 4:
                    robot_target.y -= movementFactor * deltaTime;
                    break;
                case 5:
                    robot_target.y += movementFactor * deltaTime;
                    break;
            }
        }
    }

    if (change) {
        kinematics->solve_inverse(robot_target);
    }
}

void joystick_callback(int jid, int event) {
    joysticks->query_joysticks();
}