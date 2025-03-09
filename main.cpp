#include <iomanip>
#include <thread>

#include <signal.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <hidapi/hidapi.h>

#include "include/common.h"
#include "include/camera.h"
#include "include/texture.h"
#include "include/mesh.h"
#include "include/shader.h"
#include "include/shader_program.h"
#include "include/text.h"
#include "include/ui_element.h"
#include "include/ui_text.h"

struct shader_text_t;
struct shader_materials_t;
template<typename mesh_base = mesh_t>
struct segment_T;
struct kinematics_t;
struct debug_object_t;
struct debug_info_t;
struct ui_text_t;
struct ui_slider_t;
struct ui_toggle_t;
struct joystick_t;
struct robot_interface_t;

using segment_t = segment_T<>;

texture_t *textTexture, *mainTexture;
shader_t *mainVertexShader, *mainFragmentShader;
shader_t *textVertexShader, *textFragmentShader;
shader_text_t *textProgram;
shader_materials_t *mainProgram;
material_t *robotMaterial;
camera_t *camera;
segment_t *sBase, *s6, *s5, *s4, *s3, *s2, *s1;
std::vector<segment_t*> segments;
std::vector<segment_t*> servo_segments;
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

struct shader_materials_t : public shaderProgram_t {
    shader_materials_t(shaderProgram_t prg)
    :shaderProgram_t(prg),material(0),light(0) { }

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
        shaderProgram_t::use();

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

struct shader_text_t : public shaderProgram_t {
    shader_text_t(shaderProgram_t prg)
    :shaderProgram_t(prg),mixFactor(0.0) { }

    float mixFactor;

    void use() override {
        shaderProgram_t::use();

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glDisable(GL_CULL_FACE);        

        set_m4("projection", glm::mat4(1.) * viewport_inversion);
        set_f("mixFactor", mixFactor);
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

template<typename mesh_base>
struct segment_T {
    segment_T(segment_T *parent, glm::vec3 rotation_axis, glm::vec3 initial_direction, float length, int servo_num) {
        this->parent = parent;
        this->rotation_axis = rotation_axis;
        this->initial_direction = initial_direction;
        this->length = length;
        this->servo_num = servo_num;
        this->rotation = 0.0f;
        this->mesh = new mesh_base();
    }

    ~segment_T() {
        if (this->mesh) {
            ~this->mesh();
            delete this->mesh;
        }
        this->mesh = nullptr;
    }

    static glm::vec3 matrix_to_vector(glm::mat4 mat) {
        return glm::normalize(glm::vec3(mat[2]));
    }

    static inline constexpr float clamp(const float &v, const float &min, const float &max) {
        float ret = v;
        const float r = max - min;
        while (ret < min) ret += r;
        while (ret > max) ret -= r;
        return ret;
    }

    static inline constexpr float clip(const float &v, const float &min, const float &max) {
        float ret = v;
        if (ret < min)
            ret = min;
        if (ret > max)
            ret = max;
        return ret;
    }

    inline float get_clamped_rotation(bool allow_interpolate = false) {
        return clamp(allow_interpolate ? get_rotation() : rotation, -180, 180);
    }

    inline float get_rotation(bool allow_interpolate = true);

    float get_length() {
        return length * model_scale;
    }

    static glm::mat4 vector_to_matrix(glm::vec3 vec) {
        glm::vec4 hyp_vec = glm::vec4(vec.x, vec.y, vec.z, 1.0f);
        return glm::mat4(1.0f) * glm::mat4(hyp_vec,hyp_vec,hyp_vec,hyp_vec);
    }

    static glm::vec3 rotate_vector_3d(glm::vec3 vec, glm::vec3 axis, float degrees) {
        float radians = glm::radians(degrees);
        glm::mat4 rtn_matrix = vector_to_matrix(vec);

        return matrix_to_vector(glm::rotate(rtn_matrix, radians, axis));
    }

    glm::mat4 get_rotation_matrix(bool allow_interpolate = true) {
        if (!parent)
            return glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(x_axis));

        return glm::rotate(parent->get_rotation_matrix(allow_interpolate), glm::radians(get_rotation(allow_interpolate)), rotation_axis);
    }

    glm::mat4 get_model_transform(bool allow_interpolate = true) {
        glm::vec3 origin = get_origin();
        glm::mat4 matrix(1.);
        
        matrix = glm::translate(matrix, origin);
        matrix *= get_rotation_matrix(allow_interpolate);
        matrix = glm::scale(matrix, glm::vec3(model_scale));
        
        return matrix;        
    }

    glm::vec3 get_segment_vector(bool allow_interpolate = true) {
        return matrix_to_vector(get_rotation_matrix(allow_interpolate)) * get_length();
    }

    glm::vec3 get_origin(bool allow_interpolate = true) {
        if (!parent) {
            assert(mesh && "Mesh null\n");
            return mesh->position;
        }

        if (servo_num == 5) { //shift forward just for this servo
            glm::mat4 iden = parent->get_rotation_matrix(allow_interpolate);
            auto trn = glm::vec3(2.54f * -model_scale, 0., 0.);
            iden = glm::translate(iden, trn);
            iden = glm::rotate(iden, glm::radians(90.0f), {0,0,1});

            return parent->get_segment_vector(allow_interpolate) + 
                   parent->get_origin(allow_interpolate) + 
                   glm::vec3(iden[3]);
        }

        return parent->get_segment_vector(allow_interpolate) + parent->get_origin(allow_interpolate);
    }

    void renderVector(glm::vec3 origin, glm::vec3 end) {
        debug_objects->add_line(origin, end);
    }

    void renderMatrix(glm::vec3 origin, glm::mat4 mat) {
        glm::vec4 m = glm::vec4(origin.x, origin.y, origin.z, 0);

        renderVector(origin, m + mat[0]);
        renderVector(origin, m + mat[1]);
        renderVector(origin, m + mat[2]);
    }

    void renderDebug() {
        glm::vec3 origin = get_origin();
        debug_objects->add_sphere(origin, model_scale);
        debug_objects->add_sphere(origin + get_segment_vector(), model_scale);
        renderVector(origin, origin + get_segment_vector());

        auto rot_mat = get_rotation_matrix();
        auto tran_rot_mat = glm::translate(rot_mat, origin);

        renderMatrix(origin, tran_rot_mat);
    }

    void render() {
        mainProgram->use();
        mainProgram->set_camera(camera, glm::mat4(1.0f));
        if (debug_mode)
            renderDebug();
        mainProgram->set_camera(camera, get_model_transform());
        if (debug_pedantic)
            mainProgram->set_v3("light.ambient", debug_color);
        mesh->render();
    }

    virtual bool load(const char *path) {
        assert(mesh && "Mesh null");
        return mesh->load(path);
    }

    float model_scale = 0.1;
    glm::vec3 direction, rotation_axis, initial_direction;
    segment_t *parent;
    glm::vec3 debug_color;
    float length, rotation;
    int servo_num;
    mesh_t *mesh;
};

struct ui_slider_t : public ui_element_t {
    using ui_slider_v = float;
    using callback_t = std::function<void(ui_slider_t*, ui_slider_v)>;

    ui_slider_v min, max, value, drag_value, initial_value;
    bool cursor_drag, limit, skip_text;
    glm::vec2 cursor_start, drag_start;
    callback_t value_change_callback;

    ui_text_t *title_text, *min_text, *max_text, *value_text;
    float title_pos_y;
    float text_pos_y;
    std::string title_cached;

    ui_slider_t(GLFWwindow *window, glm::vec4 XYWH, ui_slider_v min, ui_slider_v max, ui_slider_v value, std::string title = std::string(), bool limit = true, callback_t value_change_callback = callback_t(), bool skip_text = false, bool hidden = false)
    :ui_element_t(window, XYWH),min(min),max(max),value(value),initial_value(value),cursor_drag(false),limit(limit),skip_text(skip_text),title_cached(title) {
        title_text = min_text = max_text = value_text = 0;
        title_pos_y = text_pos_y = 0;
        this->hidden = hidden;

        if (!skip_text) {
            if (title.size() > 0) {
                title_text = add_child(new ui_text_t(window, XYWH));
            }
            min_text = add_child(new ui_text_t(window, XYWH));
            max_text = add_child(new ui_text_t(window, XYWH));
            value_text = add_child(new ui_text_t(window, XYWH));
        }

        arrange_text();

        this->value_change_callback = value_change_callback;
    }

    virtual void set_min(ui_slider_v min) {
        this->min = min;

        if (min_text) {
            min_text->XYWH = XYWH + glm::vec4(-value_subpos_x,text_pos_y,0,0);
            min_text->set_string(vtos(min));
        }

        modified = true;
    }

    virtual void set_max(ui_slider_v max) {
        this->max = max;

        if (max_text) {
            max_text->XYWH = XYWH + glm::vec4(XYWH[2]-value_subpos_x,text_pos_y,0,0);
            max_text->set_string(vtos(max));
        }

        modified = true;
    }

    virtual std::string vtos(ui_slider_v v) {
        const int buflen = 100;
        char buf[buflen];
        snprintf(buf, buflen, "%.1f", v);
        return std::string(buf);
    }

    virtual void set_value(ui_slider_v v, bool callback = true) {
        ui_slider_v _vset = v;

        if (limit)
            _vset = clip(_vset, min, max);
            
        this->value = _vset;
        if (value_text) {
            auto slider = get_slider_position();
            value_text->XYWH = glm::vec4(slider.x, text_pos_y + XYWH.y - value_subpos_y, 0,0);
            value_text->set_string(vtos(value));
        }

        if (value_change_callback && callback)
            value_change_callback(this, this->value);

        modified = true;
    }

    virtual void set_title(std::string title = std::string()) {
        if (title_text) {
            title_pos_y = -title_height;
            title_text->XYWH = XYWH + glm::vec4(0,title_pos_y,0,title_height);
            if (title.size() > 0)
                title_cached = title;
            title_text->set_string(title_cached);
        }
    }

    virtual bool arrange_text() {
        if (skip_text) {
            return glsuccess;
        }

        if (title_cached.size() > 0) {
            set_title();
            text_pos_y = title_pos_y + title_height - value_subpos_y;
        } else {
            text_pos_y = -value_subpos_y;
        }

        set_min(min);
        set_max(max);
        set_value(value);

        return glsuccess;
    }

    bool reset() override {
        //Resets children text
        ui_element_t::reset();

        if (skip_text)
            return glsuccess;

        //Sets the text after a reset
        value = initial_value;
        arrange_text();

        return glsuccess;
    }

    std::string get_element_name() override {
        return "ui_slider_t";
    }

    ui_slider_v clip(ui_slider_v val, ui_slider_v b1, ui_slider_v b2) {
        ui_slider_v min = std::min(b1, b2);
        ui_slider_v max = std::max(b1, b2);

        if (val < min)
            return min;
        if (val > max)
            return max;
        return val;

        //return std::min(std::max(value, min), max);
    }

    bool onCursor(double x, double y) override {
        if (hidden)
            return glsuccess;

        if (cursor_drag) {
            bool shift = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS);
            float fact = shift ? precise_factor : 1.;

            auto current_cursor = get_cursor_relative();
            ui_slider_v v_diff = (current_cursor.x - cursor_start.x);

            if (v_diff * v_diff < granularity * granularity) 
                return glcaught; //don't update slider when difference is negligible

            cursor_start = current_cursor;

            ui_slider_v v_raw = v_diff / (XYWH[2] * fact);
            ui_slider_v v_map = (v_raw * (max - min)) + min;
            ui_slider_v v_range = (max - min);
            //printf("set slider <%f,%f>\n", v_raw, v_map);
            //set_value( v_map);
            set_value(v_range * v_raw + value);

            return glcaught;
        }
        return ui_element_t::onCursor(x,y);
    }

    glm::vec4 get_slider_position() {
        ui_slider_v bound = std::min(std::max(value, min), max);
        ui_slider_v min = std::min(this->max, this->min);
        ui_slider_v max = std::max(this->max, this->min);

        auto in = XYWH;
        ui_slider_v norm = ((bound - min) / (max - min));

        auto mp = get_midpoint_relative();
        auto size = get_size();
        auto pos = get_position();
        auto i_size = mp * glm::vec2(0.5);
        auto half_i_size = i_size / glm::vec2(2.);
        
        //return glm::vec4(norm * size.x + pos.x - half_i_size.x, mp.y - half_i_size.y, i_size.x, i_size.y);

        auto _pos = (norm * in[2]) + in[0];
        auto itemh = in[3] * slider_frac_h;
        //auto itemw = in[2] * slider_frac_w;
        auto itemw = itemh * slider_frac_w;
        auto itemwh = glm::vec2(itemw, itemh); //.1

        auto mp_y = (in[3] / 2.) + in[1];
        auto itemhf = itemwh / glm::vec2(2.);

        //printf("norm: %f, pos: %f, itemh: %f, mp_y: %f, value: %f, min: %f, max: %f, bound: %f, min: %f, max: %f\n", norm, pos, itemh, mp_y, value, min, max, bound, this->min, this->max);

        return glm::vec4(_pos - itemhf.x, mp_y - itemhf.y, itemwh.x, itemwh.y);
    }

    bool onMouse(int button, int action, int mods) override {
        if (hidden)
            return glsuccess;

        if (cursor_drag && button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
            cursor_drag = false;
            set_value(drag_value);
            return glcaught;
        }

        if (is_cursor_bound() && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            cursor_drag = true;
            cursor_start = get_cursor_relative();
            drag_start = cursor_start;
            drag_value = value;
            return glcaught;
        } else {
            cursor_drag = false;
        }
        return ui_element_t::onMouse(button, action, mods);
    }

    bool mesh() override {
        modified = false;
        vertexCount = 0;

        text_t *buffer = new text_t[1 * 18];

        auto mp = get_midpoint_relative();
        auto pos = get_position();
        auto size = get_size();

        glm::vec4 slider_bar_position = get_slider_position();
        float slider_range_height = slider_bar_position[3] / 4.;
        glm::vec4 slider_range_position = {pos.x,pos.y+mp.y-(slider_range_height/2),size.x,slider_range_height};

        glm::vec4 slider_range_color(0,0,0,.75); //75% alpha black
        glm::vec4 slider_bar_color(0,0,.5,1); //50% blue

        //background
        //add_rect(buffer, vertexCount, XYWH, slider_range_color, true);
        //slider range
        add_rect(buffer, vertexCount, slider_range_position, slider_range_color, true);
        //slider
        add_rect(buffer, vertexCount, slider_bar_position, slider_bar_color, true);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof * buffer, buffer, GL_STATIC_DRAW);

        buffer->set_attrib_pointers();

        delete [] buffer;

        return glsuccess;
    }

    bool render() override {
        if (modified)
            mesh();

        if (vertexCount < 1 || hidden)
            return glsuccess;

        textProgram->use();
        textProgram->set_f("mixFactor", 1.0);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        glBindVertexArray(0);

        ui_element_t::render();
        
        return glsuccess;
    }
};

struct ui_toggle_t : public ui_element_t {
    using toggle_callback_t = std::function<void(ui_toggle_t*,bool)>;
    toggle_callback_t toggle_callback;

    bool toggle_state, held, initial_state;
    std::string cached_title;
    ui_text_t *title_text;

    ui_toggle_t(GLFWwindow *window, glm::vec4 XYWH, std::string title = std::string(), bool initial_state = false, toggle_callback_t toggle_callback = toggle_callback_t())
    :ui_element_t(window, XYWH),toggle_state(initial_state),initial_state(initial_state),held(false),cached_title(title),toggle_callback(toggle_callback) {
        if (cached_title.size() > 0)
            title_text = add_child(new ui_text_t(window, XYWH - glm::vec4(title_height/4.,title_height,0,0), cached_title));
    }

    bool onMouse(int key, int action, int mods) override {
        if (is_cursor_bound() && key == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_RELEASE) {
                set_toggle();
                set_held(false);
            } else
            if (action == GLFW_PRESS) {
                set_held(true);
            }
            return glcaught;
        }

        return glsuccess;
    }

    void set_toggle() {
        set_state(!toggle_state);
    }

    void set_held(bool state) {
        if (held != state) {
            modified = true;
            held = state;
        }
    }

    void set_state(bool state) {
        modified = true;
        toggle_state = state;
        if (toggle_callback)
            toggle_callback(this, toggle_state);
    }

    bool reset() override {
        ui_element_t::reset();

        set_held(false);
        set_state(initial_state);
        if (title_text)
            title_text->set_string(cached_title);

        return glsuccess;
    }

    bool mesh() override {
        modified = false;
        vertexCount = 0;

        text_t *buffer = new text_t[2 * 6];

        glm::vec4 button_held_color(0,0,0.5,1.);
        glm::vec4 button_on_color(0,0,1.0,1.);
        glm::vec4 button_off_color(0,0,0.1,1.);

        glm::vec4 color = (held ? button_held_color : (toggle_state ? button_on_color : button_off_color));

        add_rect(buffer, vertexCount, XYWH, color, true);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof * buffer, buffer, GL_STATIC_DRAW);

        buffer->set_attrib_pointers();

        delete [] buffer;

        return glsuccess;
    }

    bool render() override {
        if (modified)
            mesh();

        if (vertexCount < 1)
            return glsuccess;

        textProgram->use();
        textProgram->set_f("mixFactor", 1.0);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        glBindVertexArray(0);

        ui_element_t::render();

        return glsuccess;
    }
};

struct kinematics_t {
    static glm::vec3 map_to_xy(glm::vec3 pos, float r = 0.0f, glm::vec3 a = -z_axis, glm::vec3 o = glm::vec3(0.0f), glm::vec3 t = glm::vec3(0.0f)) {
        auto matrix = glm::mat4(1.0);
        matrix = glm::rotate(matrix, glm::radians(r), a);
        auto _pos = pos - o;

        return glm::vec3(matrix * glm::vec4(_pos.x,_pos.y,_pos.z,1)) + t;
    }

    bool solve_inverse(vec3_d coordsIn) {
        auto isnot_real = [](float x){
            return (isinf(x) || isnan(x));
        };

        bool calculation_failure = false;

        auto target_coords = coordsIn;
        target_coords.z = -target_coords.z;
        auto target_2d = glm::vec2(target_coords.x, target_coords.z);

        float rot_out[5];

        for (int i = 0; i < segments.size(); i++)
            rot_out[i] = segments[i]->rotation;

        // solve base rotation
        auto seg_6 = s6->get_origin();
        auto seg2d_6 = glm::vec2(seg_6.x, seg_6.z);

        auto dif_6 = target_2d - seg2d_6;
        auto atan2_6 = atan2(dif_6[1], dif_6[0]);
        auto norm_6 = atan2_6 / M_PI;
        auto rot_6 = (norm_6 + 1.0f) / 2.0f;
        auto deg_6 = rot_6 * 360.0f;
        auto serv_6 = rot_6;

        glm::vec3 seg_5 = s5->get_origin();
        glm::vec3 seg_5_real = glm::vec3(seg_5.x, seg_5.z, seg_5.y);
        glm::vec3 target_for_calc = target_coords;

        auto target_pl3d = map_to_xy(target_for_calc, deg_6, y_axis, seg_5);
        auto target_pl2d = glm::vec2(target_pl3d.x, target_pl3d.y);
        auto plo3d = glm::vec3(0.0f);
        auto plo2d = glm::vec2(0.0f);
        auto pl3d = glm::vec3(500.0f, 0.0f, 0.0f);
        auto pl2d = glm::vec2(pl3d);
        auto s2d = glm::vec2(500);

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

        rot_out[1] = serv_6;

        if (debug_pedantic)
            printf("End rot: %.2f,%.2f,%.2f,%.2f,%.2f\n", rot_out[0], rot_out[1], rot_out[2], rot_out[3], rot_out[4]);
        for (int i = 0; i < segments.size(); i++)
            segments[i]->rotation = rot_out[i] * 360.0f;

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

    std::map<std::string, joystick_device_t> joysticks;
    
    bool pedantic_debug = false;
    bool camera_move = false;

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
                    s2->rotation += ndd.x * deltaTime;
                    s1->rotation -= ndd.y * deltaTime;
                    s1->rotation += ndd.z * deltaTime;

                    s2->rotation = s2->clip(s2->rotation, -120, 120); // because we can't check robot_interface unless refactor, works for now
                    s1->rotation = s1->clip(s1->rotation, -50, 50);
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
        for (auto &joy : joysticks)
            joy.second.update();

        process_input(deltaTime);
        set_robot(deltaTime);
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
        std::map<std::string, joystick_device_t> joys;
        static bool first_run = true;

        for (int jid = GLFW_JOYSTICK_1; jid < GLFW_JOYSTICK_LAST; jid++) {
            if (!glfwJoystickPresent(jid))
                continue;

            // Note, GUID can match two discrete joysticks from a single device

            auto kv = joystick_device_t::get_device(jid);
            auto &jd = kv.second;

            if (joysticks.contains(kv.first)) {
                joys.insert({kv.first, joysticks[kv.first]});
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
        joysticks = joys;
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

    struct robot_servo {
        robot_servo() {}
        robot_servo(int id, int min, int max, int home, int pos, int real_pos, float deg_per_second, float int_per_deg)
        :id(id),min(min),max(max),pos(pos),
        real_pos(real_pos),deg_per_second(deg_per_second),
        home(home),last_cmd(clk::now()),int_per_deg(int_per_deg) { }
        int id, min, max, home;
        // robot's real position based off a query or time-based movement interpolation
        int real_pos;
        // robot's target position, if set without last_cmd, will jerk the robot
        int pos;
        float int_per_deg, deg_per_second;
        // last time a movement was sent to the robot
        tp last_cmd;
        int min_time = 20;
        int min_diff = 1;

        template<typename T = int, typename RET = float>
        RET get_deg(T v) {
            return RET(v - home) * int_per_deg;
        }

        template<typename T = float, typename RET = int>
        RET get_int(T v) {
            return RET(v * (1.0 / int_per_deg)) + home;
        }

        template<typename RET = int>
        RET get_elapsed_time() {
            using _dur = std::chrono::duration<RET>;
            return (RET)std::chrono::duration_cast<_dur>(clk::now() - last_cmd).count();
        }

        template<typename T = int>
        T get_interpolated_pos(bool debug = false) {
            T d = pos - real_pos;

            if (abs(d) < min_diff)
                return pos;

            using _dur = std::chrono::duration<float>;
            auto t = std::chrono::duration_cast<_dur>(clk::now() - last_cmd);

            T md = t.count() * deg_per_second;
            T dir = d > 0 ? 1 : -1;
            T intp = dir * md;

            if (debug && debug_pedantic) {
                if (std::is_same_v<int, T>) {
                    fprintf(stderr, "gip id:%i t:%f d:%i md:%i dir:%i pos:%i intp:%i intp + real_pos:%i\n", (int)id, t.count(), (int)d, (int)md, (int)dir, (int)pos, (int)intp, (int)(intp + real_pos));
                }
            }

            if (abs(intp) > abs(d))
                return pos;

            return intp + real_pos;
        }

        bool ready_for_command() {
            return get_elapsed_time() > min_time && get_interpolated_pos() == pos;
        }

        void set_pos(int v) {
            last_cmd = clk::now();
            real_pos = pos;
            pos = v;
        }
    };

    std::map<int, robot_servo> robot_servos;

    void update() {
        if (!handle && !virtual_output)
            return;

        assert(robot_servos.size() < 7 && "Need servos to update\n");

        std::vector<std::pair<int,int>> cmds;

        static tp last_batch = clk::now();
        tp now_batch = clk::now();
        static bool constant_speed = false;
        static int u_period = 10;
        static int m_period = 200;
        static int t_overlap = 0;
        int r_period = int(std::chrono::duration_cast<dur>(now_batch - last_batch).count()) + t_overlap;

        if (!constant_speed && r_period < u_period)
            return;

        if (!constant_speed && r_period > m_period)
            r_period = m_period;

        for (auto *sv : servo_segments) {
            auto &rs = robot_servos[sv->servo_num];

            float targetf = sv->get_clamped_rotation();
            int targeti = rs.get_int(targetf);

            //assert(targeti >= rs.min && targeti <= rs.max && "Position out of bounds");
            if (targeti < rs.min || targeti > rs.max) {
                //fprintf(stderr, "Clamp position %i < %i < %i\n", rs.min, targeti, rs.max);
                if (targeti < rs.min)
                    targeti = rs.min;
                if (targeti > rs.max)
                    targeti = rs.max;
            }

            int initialp = rs.real_pos;
            float initialpf = rs.get_deg<int, float>(initialp);
            int intrp = rs.get_interpolated_pos();
            int dist = targeti - intrp;

            if (constant_speed)
            { // constant speed
            if (abs(dist) < rs.min_diff) {
                //if (rs.id == 5)
                //    fprintf(stderr, "Skip sending command to s%i (%i - %i)\n", rs.id, intrp, targeti);
                continue;
            }
            if (!rs.ready_for_command()) {
                //if (rs.id == 6)
                //    fprintf(stderr, "Servo not ready s%i\n", rs.id);
                continue;
            }

            //rs.get_interpolated_pos(true);

            int time_to_complete = (fabs(dist) / rs.deg_per_second) * 1000;
            if (debug_pedantic)
                fprintf(stderr, "Send constant speed s%i (%i - (%i %.2f) = %i, %i ms)\n", rs.id, intrp, targeti, targetf, dist, time_to_complete);
            rs.set_pos(targeti);
            set_servos({{rs.id, targeti}}, time_to_complete);
            //cmds.push_back({rs.id, targeti});
            } // constant time
            else 
            {
            rs.last_cmd = now_batch;

            if (abs(dist) < rs.min_diff && abs(dist) < 1) {
                rs.pos = targeti;
                rs.real_pos = targeti;
                continue;
            }

            int mv = (r_period/1000.0f) * rs.deg_per_second;
            int mvdist = rs.real_pos - intrp;
            float accel = mvdist / (float)mv;
            float jerk = (mvdist * (float)mv) / (float)mv;
            int rintrp = intrp;

            
            if (abs(jerk) > mv / 2 && abs(mvdist) > 0) {
                intrp += (mvdist * .5); //limit jerk; to-do velocity
            } else
            if (abs(dist) < mv / 2 && abs(jerk) < mv / 4) {
                intrp = targeti;
                if (debug_mode)
                    fprintf(stderr, "Force set targeti\n");
            }


            //rs.set_pos(targeti);
            rs.pos = targeti;
            rs.real_pos = intrp;
            
            if (debug_mode)
                fprintf(stderr, "Send constant time s%i (%i/initialp -> %i/rintrp (jerk comp %i/intrp)) (%i/mvdist) (%i/targeti %.2f/targetf : %.2f/initialpf) = %i/dist (%i r_period/ms %i mv/intpersec) accel %.2f jerk %.2f\n", rs.id, initialp, rintrp, intrp, mvdist, targeti, targetf, initialpf, dist, r_period, mv, accel, jerk);
            
            cmds.push_back({rs.id, intrp});
            }
        }

        if (!constant_speed) {
            if (cmds.size() > 0) {
                set_servos(cmds, r_period);
                last_batch = now_batch;
            }
        }
        //if (cmds.size() > 0)
        //    set_servos(cmds, 1000);
        //for (auto &cmd : cmds)
            //set_servos({cmd}, 1000);
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

        auto now_time = clk::now();
        for (int i = 0; i < count; i++) {
            int index = 5 + 3 * i;
            int id = ret[index];
            int pos = (ret[index + 2] << 8) | ret[index + 1];
            robot_servos[id].real_pos = (unsigned short)(pos);
            if (set_pos)
                robot_servos[id].pos = (unsigned short)(pos);
            robot_servos[id].last_cmd = now_time;
            if (debug_pedantic)
                fprintf(stderr, "s%i r%i p%i\n", id, robot_servos[id].real_pos, robot_servos[id].pos);
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
        /*
        https://github.com/migsdigs/Hiwonder_xArm_ESP32
        1: 0.39 sec/60deg, 160 deg - or (60*375/90/0.39) = 641.03/s
        6-2: 0.22 sec/60deg, 240 deg - or (60*375/90/0.22) = 1136.4/s
        */

        int dmin = 200;
        int dmax = 800;
        int dhome = 500;
        int dpos = 500;
        int drp = 500;
        //float dconv = 90.0f / 375.0f;
        float dconv = 240.0f / 1000.0f; //from spec
        float dps = 500.0f;
        robot_servo servos[6] = {
            { 1, 200, 850, dhome, dpos, drp, 641.0f, dconv }, // gripper
            { 2, 0, 925, dhome, dpos, drp, 700.0f, dconv }, // wrist
            { 3, 38, 1000, dhome, dpos, drp, 700.0f, dconv }, // 3
            { 4, 0, 1042, 502, dpos, drp, 700.0f, -dconv }, // 4 (inverted)
            { 5, 148, 882, 505, dpos, drp, 700.0f, dconv }, // 5
            { 6, 0, 1146, 482, dpos, drp, 700.0f, dconv }  // base
        };

        for (int i = 0; i < 6; i++)
            robot_servos.insert({servos[i].id, servos[i]});

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
        robot_servos.clear();
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
        for (auto &rs : robot_servos) {
            ret += std::format("  {}: {}\n", rs.second.id, rs.second.real_pos);
        }
        return ret;
    }
};

void joystick_t::query_robot() {
    robot_interface->read_all();
    for (auto &rs_kv : robot_interface->robot_servos) {
        auto &rs = rs_kv.second;
        fprintf(stderr, "%i: %i (%.2f deg), ", rs.id, rs.real_pos, rs.get_deg(rs.real_pos));
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

template<>
inline float segment_T<>::get_rotation(bool allow_interpolate) {
    if (model_interpolation && allow_interpolate) {
        auto &rb = robot_interface->robot_servos;
        if (rb.contains(servo_num)) {
            auto &rs = rb[servo_num];
            return rs.get_deg<float>(rs.get_interpolated_pos<float>());
        }
    }
    return rotation;
}

void set_segments_from_sliders() {
    for (int i = 0; i < servo_sliders.size() && i < servo_segments.size(); i++)
        servo_segments[i]->rotation = servo_sliders[i]->value;

    robot_target = s3->get_origin() + s3->get_segment_vector();
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
        auto &rs = robot_interface->robot_servos[sg->servo_num];
        float intrp = rs.get_interpolated_pos<float>();
        sg->rotation = rs.get_deg<float>(intrp);
        ui->set_value(sg->get_clamped_rotation(), false);
        if (debug_pedantic)
            fprintf(stderr, "%i -> %s (%f -> %f)\n", sg->servo_num, ui->title_cached.c_str(), sg->rotation, sg->get_clamped_rotation());
    }

    robot_target = s3->get_origin() + s3->get_segment_vector();
}

void set_robot_from_segments() {
    for (int i = 0; i < servo_sliders.size(); i++) {
        auto *sg = servo_segments[i];
        auto &rv = robot_interface->robot_servos;
        if (!rv.contains(sg->servo_num))
            continue;
        
        auto &rs = rv[sg->servo_num];
        rs.set_pos(rs.get_int(sg->rotation));
    }
}

std::string segment_debug_info() {
    std::string ret = "Rotation\n";

    auto &rs = robot_interface->robot_servos;

    auto get_segment = [&](segment_t *seg) {
        float cv = seg->get_clamped_rotation();
        if (rs.contains(seg->servo_num)) {
            auto &rv = rs[seg->servo_num];
            return std::format("  {}: {}  {}\n", seg->servo_num, cv, rv.get_int<float, float>(cv));
        }
        return std::format("  {}: {}\n", seg->servo_num, cv);
    };

    return ret + get_segment(s1) +
                 get_segment(s2) +
                 get_segment(s3) +
                 get_segment(s4) +
                 get_segment(s5) +
                 get_segment(s6);
}

void update_debug_info() {
    {
        const int bufsize = 1000;
        char char_buf[bufsize];

        glm::vec3 s3_t = s3->get_segment_vector() + s3->get_origin();
        
        debug_objects->add_sphere(robot_target, s3->model_scale);

        snprintf(char_buf, bufsize, 
        "%.0lf FPS\nCamera %.2f %.2f %.2f\nFacing %.2f %.2f\nTarget %lf %lf %lf\ns3 %.2f %.2f %.2f\n%s%s%s",
        fps, 
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

void calcFps() {
    frameCount++;

    double nowTime = glfwGetTime();
    deltaTime = nowTime - lastTime;
    lastTime = nowTime;

    static auto now_time = hrc::now();
    static auto diff_time = hrc::now();
    diff_time = now_time;
    now_time = hrc::now();
    deltaTime = dur(now_time - diff_time).count();


    end = hrc::now();
    duration = end - start;

    if (duration.count() >= 1.0) {
        fps = frameCount / duration.count();
        frameCount = 0;
        start = hrc::now();
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
    glfwInit();

    initial_window = {0,0,1600,900};

    window = glfwCreateWindow(initial_window[2], initial_window[3], "xArm", nullptr, nullptr);
    if (!window) {
        handle_error("failed to create glfw window");
    }
    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetJoystickCallback(joystick_callback);
    glfwSwapInterval(1);

    glfwGetWindowPos(window, &initial_window.x, &initial_window.y);
    glfwGetWindowSize(window, &initial_window[2], &initial_window[3]);

    current_window = initial_window;

    lastTime = glfwGetTime();

    signal(SIGQUIT, handle_signal);
    signal(SIGKILL, handle_signal);
    signal(SIGSTOP, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);

    return glsuccess;
}

int init() {
    camera = new camera_t();
    mainVertexShader = new shader_t("shaders/vertex.glsl", GL_VERTEX_SHADER);
    mainFragmentShader = new shader_t("shaders/fragment.glsl", GL_FRAGMENT_SHADER);
    textVertexShader = new shader_t("shaders/text_vertex_shader.glsl", GL_VERTEX_SHADER);
    textFragmentShader = new shader_t("shaders/text_fragment_shader.glsl", GL_FRAGMENT_SHADER);

    mainProgram = new shader_materials_t(shaderProgram_t(mainVertexShader, mainFragmentShader));
    textProgram = new shader_text_t(shaderProgram_t(textVertexShader, textFragmentShader));

    mainTexture = new texture_t();
    mainTexture->generate(glm::vec4(0.0f,0.0f,1.0f,1.0f));
    textTexture = new texture_t("assets/text.png");

    robotMaterial = new material_t(mainTexture,mainTexture,0.2f);
    mainProgram->set_material(robotMaterial);

    uiHandler = new ui_element_t(window, {-1.0f,-1.0f,2.0f,2.0f});
    //uiHandler->add_child(new ui_text_t(window, {0.0,0.0,.1,.1}, "Hello World!"));
    debugInfo = uiHandler->add_child(new ui_text_t(window, textProgram, textTexture, {-1.0f,-1.0f,2.0f,2.0f}, "", update_debug_info));
    debug_objects = new debug_object_t();
    ui_servo_sliders = uiHandler->add_child(new ui_element_t(window, uiHandler->XYWH));

    glm::vec4 sliderPos = {0.45, -0.95,0.5,0.1};
    glm::vec4 sliderAdd = {0.0,0.2,0,0};
    glm::vec2 sMM = {-180, 180.};
    
    int sl = 0;
    slider6 = ui_servo_sliders->add_child(new ui_slider_t(window, sliderPos, sMM.x, sMM.y, sl++, "Servo 6", true, servo_slider_update));
    slider5 = ui_servo_sliders->add_child(new ui_slider_t(window, sliderPos + (sliderAdd * glm::vec4(sl++)), sMM.x, sMM.y, 0., "Servo 5", true, servo_slider_update));
    slider4 = ui_servo_sliders->add_child(new ui_slider_t(window, sliderPos + (sliderAdd * glm::vec4(sl++)), sMM.x, sMM.y, 0., "Servo 4", true, servo_slider_update));
    slider3 = ui_servo_sliders->add_child(new ui_slider_t(window, sliderPos + (sliderAdd * glm::vec4(sl++)), sMM.x, sMM.y, 0., "Servo 3", true, servo_slider_update));
    slider2 = ui_servo_sliders->add_child(new ui_slider_t(window, sliderPos + (sliderAdd * glm::vec4(sl++)), sMM.x, sMM.y, 0., "Servo 2", true, servo_slider_update));
    slider1 = ui_servo_sliders->add_child(new ui_slider_t(window, sliderPos + (sliderAdd * glm::vec4(sl++)), sMM.x, sMM.y, 0., "Servo 1", true, servo_slider_update));
    //debugInfo = new ui_text_t(window, {0.0,0.0,1.,1.}, "Hello world 2!");
    
    slider_ambient = debugInfo->add_child(new ui_slider_t(window, sliderPos + (sliderAdd * glm::vec4(sl++)), -2., 2., .5, "Ambient Light", false));
    slider_diffuse = debugInfo->add_child(new ui_slider_t(window, sliderPos + (sliderAdd * glm::vec4(sl++)), -2., 2., 0.5, "Diffuse Light", false));
    slider_specular = debugInfo->add_child(new ui_slider_t(window, sliderPos + (sliderAdd * glm::vec4(sl++)), -2., 2., 0., "Specular Light", false));
    slider_shininess = debugInfo->add_child(new ui_slider_t(window, sliderPos + (sliderAdd * glm::vec4(sl++)), -2., 2., 0.5, "Shininess", false));

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

    bool extra_slider_hidden = true;
    for (int i = 0; i < (sizeof defaults / sizeof defaults[0]); i++) {
        int stepover = 8;
        if (i == stepover)
            sliderPos += glm::vec4(sliderPos[2]+0.1,0,0,0);
        slider_whatever.push_back(debugInfo->add_child(new ui_slider_t(window, sliderPos + (sliderAdd * glm::vec4(float(i % stepover))), defaults[i]-2., defaults[i]+2., defaults[i], "", false, update_whatever, false, extra_slider_hidden)));
    }

    auto toggle_pos = glm::vec4{-.975,.925,.05,.05};
    auto toggle_add = glm::vec4{.110, 0, 0, 0};

    debugInfo->hidden = !debug_mode;
    debugToggle = uiHandler->add_child(new ui_toggle_t(window, toggle_pos, "Debug", debug_mode, [](ui_toggle_t* ui, bool state){
        debug_mode = state;
        debugInfo->hidden = !debug_mode;
    }));
    resetToggle = debugInfo->add_child(new ui_toggle_t(window, toggle_pos += toggle_add, "Reset", false, [](ui_toggle_t *ui, bool state) {
        if (state)
            uiHandler->reset();
        reset();
    }));
    interpolatedToggle = debugInfo->add_child(new ui_toggle_t(window, toggle_pos += toggle_add, "Intrp", model_interpolation, [](ui_toggle_t *ui, bool state){
        model_interpolation = state;
    }));
    resetConnectionToggle = debugInfo->add_child(new ui_toggle_t(window, toggle_pos += toggle_add, "Conn", false, [](ui_toggle_t *ui, bool state) {
        if (state) {
            robot_interface->reset(1155, 22352);
            resetConnectionToggle->set_state(false);
        }
    }));
    pedanticToggle = debugInfo->add_child(new ui_toggle_t(window, toggle_pos += toggle_add, "Verb", debug_pedantic, [](ui_toggle_t* ui, bool state){
        debug_pedantic = state;
    }));

    sBase = new segment_t(nullptr, z_axis, z_axis, 46.19, 7);
    s6 = new segment_t(sBase, z_axis, z_axis, 35.98, 6);
    s5 = new segment_t(s6, y_axis, z_axis, 100.0, 5);
    s4 = new segment_t(s5, y_axis, z_axis, 96.0, 4);
    s3 = new segment_t(s4, y_axis, z_axis, 150.0, 3);
    s2 = new segment_t(nullptr, z_axis, z_axis, 0, 2);
    s1 = new segment_t(nullptr, z_axis, z_axis, 0, 1);

    segments = std::vector<segment_t*>({sBase, s6, s5, s4, s3});
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
        seg->rotation = 0;
    set_sliders_from_segments();
    set_robot_from_segments();
    robot_target = s3->get_segment_vector(false) + s3->get_origin(false);
}

int load() {
    if ((textTexture->load() ||
        mainProgram->load() ||
        textProgram->load()))
        handle_error("a component failed to load");

    if (sBase->load("assets/xarm-sbase.stl") ||
        //s6->load("assets/xarm-s6.stl") ||
        s6->mesh->loadObj("assets/xarm-s6.obj") ||
        s5->load("assets/xarm-s5.stl") ||
        s4->load("assets/xarm-s4.stl") ||
        s3->load("assets/xarm-s3.stl"))
        handle_error("a model failed to load");

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
        handle_error("failed to load", glfail);

    while (!glfwWindowShouldClose(window)) {
        calcFps();

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        handle_keyboard(window, deltaTime);
        joysticks->update(deltaTime * 60.0);
        robot_interface->update();

        mainProgram->use();
        mainProgram->set_camera(camera);

        //glm::vec3 diffuse(0.6f), specular(0.2f), ambient(0.0f);
        glm::vec3 diffuse(slider_diffuse->value), specular(slider_specular->value), ambient(slider_ambient->value);

        float shininess = slider_shininess->value;
        robotMaterial->shininess = shininess;

        mainProgram->set_v3("eyePos", camera->position);
        mainProgram->set_v3("light.position", glm::vec3(5.0f, 15.0f, 5.0f));
        mainProgram->set_v3("light.ambient", ambient);
        mainProgram->set_v3("light.diffuse", diffuse);
        mainProgram->set_v3("light.specular", specular);
        //mainProgram->set_f("material.shininess", 2.0f);
        mainProgram->set_f("material.shininess", shininess);

        mainProgram->set_material(robotMaterial);

        for (auto *segment : segments)
            segment->render();

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
        if (glfwGetKey(window, raise[i]) == GLFW_PRESS) {
            segments[i]->rotation += movementFactor * deltaTime;
            change_2 = true;
        }

        if (glfwGetKey(window, lower[i]) == GLFW_PRESS) {
            segments[i]->rotation -= movementFactor * deltaTime;
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