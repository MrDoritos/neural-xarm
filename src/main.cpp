#include <signal.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "xarm_common.h"
#include "primitives.h"
#include "debug_object.h"
#include "kinematics.h"
#include "segment.h"
#include "robot_interface.h"
#include "joystick.h"
#include "segment_loader.h"
#include "robot.h"

using namespace robot;

robot::Robot bot(robot::segments);

struct RobotShader : public gui::MaterialShader {
    RobotShader(const gui::MaterialShader &base):gui::MaterialShader(base) { }
    RobotShader() { }

    void use() override {
        gui::MaterialShader::use();

        glEnable(GL_MULTISAMPLE);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glFrontFace(GL_CCW);
        glCullFace(GL_BACK);
        glEnable(GL_CULL_FACE);
        glEnable(GL_ALPHA_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
};


namespace render {
    void render_vector(debug_object_t *debug_objects, glm::vec3 origin, glm::vec3 end) {
        //debug_objects->add_line(origin, end);
        debug_objects->add_arrow(origin, end);
    }

    void render_matrix(debug_object_t *debug_objects, glm::vec3 origin, glm::mat4 mat) {
        auto m = glm::vec4(origin[0], origin[1], origin[2], 0);

        for (int i  = 0; i < 3; i++)
            render_vector(debug_objects, origin, m + mat[i]);
    }

    template<typename T = robot::Segment>
    void render_segment_debug(const T* segment, debug_object_t *debug_objects, const bool &allow_interpolate = true) {
        auto origin = segment->get_origin(allow_interpolate);
        auto seg_vec = segment->get_segment_vector(allow_interpolate);
        auto rot_mat = segment->get_rotation_matrix(allow_interpolate);

        //debug_objects->add_sphere(origin, segment->model_scale);
        //debug_objects->add_sphere(origin + seg_vec, segment->model_scale);
        render_vector(debug_objects, origin, origin + seg_vec);

        auto circle_x = glm::vec3(rot_mat[0]), circle_y = glm::vec3(rot_mat[1]), circle_z = glm::vec3(rot_mat[2]);
        auto circle_dot = glm::dot(circle_x, circle_z) * float(M_PI);
        auto circle_dot2 = glm::dot(circle_x, circle_y) * float(M_PI);
        auto circle_cross = glm::cross(circle_x, circle_z);
        auto circle_cross2 = glm::cross(circle_x, circle_y);
        auto circle_mat = glm::mat4(1.0);
        //auto circle_mat = glm::translate(glm::mat4(1.0f), origin) * glm::rotate(glm::mat4(1.0), circle_dot + float(M_PI * 0.5), circle_cross);
        //auto circle_mat = glm::rotate(glm::translate(glm::mat4(1.0f), origin), circle_dot + float(M_PI * 0.5), circle_cross);
        //no//auto circle_mat = glm::translate(glm::rotate(glm::mat4(1.0), circle_dot + float(M_PI * 0.5), circle_cross), origin);
        circle_mat = glm::rotate(circle_mat, circle_dot2 + float(M_PI * 0.5), {0,0,1.0f});
        circle_mat = glm::rotate(circle_mat, circle_dot + float(M_PI * 0.5), {0,1.0f,0});
        
        circle_mat[3] = glm::vec4(origin, 0.0f);
        debug_objects->add_circle(circle_mat, segment->get_length());

        auto tran_rot_mat = glm::translate(rot_mat, origin);

        render_matrix(debug_objects, origin, tran_rot_mat);
    }

    template<typename T = robot::Segment>
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

    template<typename T = robot::Segment>
    void render_segments(const std::vector<T*> &segments, shader_program_t *program, camera_t *camera, const bool &allow_interpolate = true) {
        program->use();
        //for (T* segment : segments)
        //    render_segment(segment, program, camera, allow_interpolate);
        for (T *segment : bot.get_visible_segments())
            render_segment(segment, program, camera, allow_interpolate);
    }
}

void set_segments_from_sliders() {
    int i = 0;
    for (auto *seg : bot.get_slider_segments())
        seg->set_rotation_bound(servo_sliders[i++]->value);

    robot_target = segments.back()->get_end_position(false);
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
    int i = 0;
    for (auto *seg : bot.get_slider_segments())
        servo_sliders[i++]->set_value(seg->get_clamped_rotation(), false);
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

    robot_target = segments.back()->get_end_position(false);
}

void set_robot_from_segments() {
    for (auto *seg : bot.get_segments())
        seg->set_servo(seg->get_servo_interpolated());
}

std::string segment_debug_info() {
    std::string ret = "";
    ret += std::format("{:>7} {: >12s} {: >13s}\n", "Servos:", "Interpolated", "Immediate");

    auto get_segment = [&](robot::Segment *seg) {
        return std::format("{:>3}: {:>9.2f} {:>5} {:>7.2f} {:>5} force {:>5.2f} total {:>5.2f}\n  s-load {:>3.0f}% z-load {:>3.0f}% torque {:>5.2f} total {:>5.2f}\n", 
            seg->servo_num, 
            seg->get_servo_interpolated_degrees(), 
            seg->get_servo_interpolated(), 
            seg->get_servo_degrees(), 
            seg->get_servo(), 
            seg->get_self_force().y, 
            seg->get_total_force().y, 
            seg->get_servo_load()*100.0, 
            seg->get_axis_load(glm::normalize(seg->get_total_force()))*100.0,
            seg->get_self_torque(true, seg->get_origin()).y,
            seg->get_total_torque().y
        );
    };

    for (auto *seg : bot.get_segments())
        ret += get_segment(seg);

    return ret;
}

void update_debug_info() {
    {
        const int bufsize = 2000;
        char char_buf[bufsize];

        Segment *s3 = segments.back();
        glm::vec3 s3_t = s3->get_end_position(false);

        debug_objects->add_sphere(robot_target, s3->model_scale);

        const auto vtos = [](const glm::vec4 &vec) {
            return std::format("{:>6.2f} {:>6.2f} {:>6.2f} {:>6.2f}\n", vec.x, vec.y, vec.z, vec.w);
        };

        const auto mtos = [vtos](const glm::mat4 &matrix) {
            return vtos(matrix[0]) + vtos(matrix[1]) + vtos(matrix[2]) + vtos(matrix[3]);
        };

        snprintf(char_buf, bufsize, 
        "%.0lf FPS %.2lf ms\nCamera %.2f %.2f %.2f\nFacing %.2f %.2f\nTarget %lf %lf %lf\ns3 %.2f %.2f %.2f\n%s%s%s",
        frametime.get_fps(), frametime.get_ms(), 
        camera->position.x, camera->position.y, camera->position.z,
        camera->yaw,camera->pitch,
        robot_target.x, robot_target.y, robot_target.z,
        s3_t.x,s3_t.y,s3_t.z,
        joysticks->get_debug_info().c_str(),
        segment_debug_info().c_str(),
        robot_interface->get_debug_info().c_str()
        );
        debugInfo->set_string(&char_buf[0]);
    }
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
    debug_mode = true;

    camera = new camera_t();

    mainTexture = new texture_t();
    textTexture = new texture_t();
    circleTexture = new texture_t();
    arrowTexture = new texture_t;
    robotMaterial = new material_t(mainTexture,mainTexture,1.0f);

    mainVertexShader = new gui::VertexShader;
    mainFragmentShader = new gui::FragmentShader;
    textVertexShader = new gui::VertexShader;
    textFragmentShader = new gui::FragmentShader;

    mainProgram = new RobotShader(gui::MaterialShader(robotMaterial, new light_t({5.0f,15.0f,5.0f},{.5,.5,.5},{0,0,0},{0,0,0}), mainVertexShader, mainFragmentShader));
    textProgram = new gui::UIShader(textVertexShader, textFragmentShader);

    uiHandler = new ui_element_t(window, {-1.0f,-1.0f,2.0f,2.0f});
    debugInfo = uiHandler->add_child(new ui_text_t(window, textProgram, textTexture, {-1.0f,-1.0f,2.0f,2.0f}, "", update_debug_info));
    debug_objects = new debug_object_t(camera, mainProgram, circleTexture);
    ui_servo_sliders = uiHandler->add_child(new ui_element_t(window, uiHandler->XYWH));

    glm::vec4 sliderPos = glm::vec4{-0.95,-.2,0,0} + glm::vec4{0,0,.25,0.1};
    glm::vec4 sliderAdd = {0,.105,0,0};

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
            robot_interface->reset();
            resetConnectionToggle->set_state(false);
        }
    }));
    pedanticToggle = debugInfo->add_child(new ui_toggle_t(window, textProgram, textTexture, toggle_pos += toggle_add, "Verb", debug_pedantic, [](ui_toggle_t* ui, bool state){
        debug_pedantic = state;
    }));

    kinematics = new robot::Kinematics;
    robot_interface = new robot::RobotInterface(true);
    joysticks = new robot::Joystick(robot_interface, camera, kinematics);

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
    //robot_target = s3->get_segment_vector(false) + s3->get_origin(false);
    robot_target = segments.back()->get_end_position(false);
}

int make_sliders() {
    glm::vec4 sliderPos = {0.45, -0.95,0.5,0.1};
    glm::vec4 sliderAdd = {0.0,0.2,0,0};
    glm::vec2 sMM = {-180, 180.};
    
    int sl = 0;
    //for (auto *seg : robot::segments)
    //    if (seg->add_slider)
    for (auto *seg : bot.get_slider_segments())
            servo_sliders.push_back(ui_servo_sliders->add_child(new ui_slider_t(window, robot::textProgram, robot::textTexture, sliderPos + (sliderAdd * glm::vec4(sl++)), sMM.x, sMM.y, 0., std::format("Servo {}", seg->servo_num), true, servo_slider_update)));

    slider_ambient = debugInfo->add_child(new ui_slider_t(window, textProgram, textTexture, sliderPos + (sliderAdd * glm::vec4(sl++)), -2., 2., -0.3, "Ambient Light", false));
    slider_diffuse = debugInfo->add_child(new ui_slider_t(window, textProgram, textTexture, sliderPos + (sliderAdd * glm::vec4(sl++)), -2., 2., 1.5, "Diffuse Light", false));
    slider_specular = debugInfo->add_child(new ui_slider_t(window, textProgram, textTexture, sliderPos + (sliderAdd * glm::vec4(sl++)), -2., 2., -0.4, "Specular Light", false));
    slider_shininess = debugInfo->add_child(new ui_slider_t(window, textProgram, textTexture, sliderPos + (sliderAdd * glm::vec4(sl++)), -2., 2., 0.6, "Shininess", false));

    return glsuccess;
}

int load() {
    if (mainTexture->generate(glm::vec4(0.0f,0.0f,0.0f,1.0f)) ||
        textTexture->load("assets/text.png") ||
        circleTexture->load("assets/circle.png") ||
        arrowTexture->load("assets/arrow.png"))
        handle_error("Failed to load textures");

    if (mainVertexShader->load("assets/shaders/vertex.glsl") ||
        mainFragmentShader->load("assets/shaders/fragment.glsl") ||
        textVertexShader->load("assets/shaders/text_vertex_shader.glsl") ||
        textFragmentShader->load("assets/shaders/text_fragment_shader.glsl"))
        handle_error("Failed to load shaders");

    if ((mainProgram->load() ||
        textProgram->load()))
        handle_error("Failed to compile shaders");

    int dhome = 500;
    int dpos = 500;
    int drp = 500;
    float dconv = 240.0f / 1000.0f;

    robot::Segment::robot_servo_type servo_vals[7] = {
        {},
        { 6, 0, 1146, 482, dpos, drp, 700.0f, dconv },  // base
        { 5, 148, 882, 505, dpos, drp, 700.0f, dconv }, // 5
        { 4, 0, 1042, 502, dpos, drp, 700.0f, -dconv }, // 4 (inverted)
        { 3, 38, 1000, dhome, dpos, drp, 700.0f, dconv }, // 3
        { 2, 0, 925, dhome, dpos, drp, 700.0f, dconv }, // wrist
        { 1, 200, 850, dhome, dpos, drp, 641.0f, dconv } // gripper
    };

    if (segment_loader->load())
        handle_error("Failed to load segments");

    if (segment_loader->load_meshes())
        handle_error("Failed to generate mesh buffers");

    segment_loader->set(meshes, segments, visible_segments, servo_segments);

    if (make_sliders())
        handle_error("Failed to make sliders");

    reset();
    uiHandler->load();
    debug_objects->load();
    framebuffer_size_callback(window, current_window[2], current_window[3]);

    joysticks->query_joysticks();
    robot_interface->open(1155, 22352);
    set_segments_from_robot();
    if (debug_pedantic)
        fprintf(stderr, "robot_target: %lf %lf %lf\n", robot_target[0], robot_target[1], robot_target[2]);

    return glsuccess;
}

int parse_args(int argc, char **argv) {
    robot::segment_loader = new robot::SegmentLoader("assets/xarm");
    
    if (argc < 2)
        return glsuccess;

    new (robot::segment_loader)robot::SegmentLoader(argv[1]);

    return glsuccess;
}

int main(int argc, char **argv) {
    if (parse_args(argc, argv)
        || init_context()
        || init()
        || load())
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

        glm::vec3 diffuse(slider_diffuse->value), specular(slider_specular->value), ambient(slider_ambient->value);

        robotMaterial->shininess = slider_shininess->value;

        mainProgram->set_eye_position(camera->position);
        mainProgram->set_light({{5.0f,15.0f,5.0f}, ambient, diffuse, specular});

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
    camera->onFramebuffer(window, width, height);
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
    robot::Segment *segs[4];
    std::copy(segments.begin()+1,segments.begin()+5,segs);

    float movementFactor = movementSpeed;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        movementFactor = rapidSpeed;

    bool change_2 = false;

    for (int i = 0; i < 4; i++) {
        auto *seg = segs[i];
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
        //robot_target = s3->get_segment_vector() + s3->get_origin();
        robot_target = segments.back()->get_end_position(true);
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