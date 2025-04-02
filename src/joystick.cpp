#include "joystick.h"
#include "robot_interface.h"

#include <thread>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

int JoystickDevice::get_button(int button) {
    if (state.buttons[button]) {
        if (held_buttons[button])
            return GLFW_REPEAT;
        held_buttons[button] = 1;
        return GLFW_PRESS;
    }
    held_buttons[button] = 0;
    return 0;
}

void JoystickDevice::set_deadzones() {
    assert(deadzones.size() == axis_count && "Deadzone array length and axis_count\n");
    memcpy(deadzones.data(), &state.axes, axis_count * sizeof deadzones[0]);
}

void JoystickDevice::update() {
    if (!glfwGetGamepadState(jid, &state)) {
        const float *axes = glfwGetJoystickAxes(jid, &axis_count);
        const unsigned char *buttons = glfwGetJoystickButtons(jid, &button_count);

        if (axis_count < 6 || axis_count > 6) {
            if (axis_count < 6)
                return;
        }

        memcpy(&state.axes, axes, sizeof state.axes);
        memcpy(&state.buttons, buttons, sizeof state.buttons);

        // remap for the controller im using
        std::swap(state.axes[2], state.axes[4]);
        std::swap(state.axes[3], state.axes[2]);
    }    
}

std::pair<std::string, JoystickDevice> JoystickDevice::get_device(int jid) {
    std::pair<std::string, JoystickDevice> kv;

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





std::map<int, std::string> Joystick::button_mapping = {
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

std::map<int, std::string> Joystick::axis_mapping = {
    {GLFW_GAMEPAD_AXIS_LEFT_X, "Left X"},
    {GLFW_GAMEPAD_AXIS_LEFT_Y, "Left Y"},
    {GLFW_GAMEPAD_AXIS_RIGHT_X, "Right X"},
    {GLFW_GAMEPAD_AXIS_RIGHT_Y, "Right Y"},
    {GLFW_GAMEPAD_AXIS_LEFT_TRIGGER, "Left Trig"},
    {GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER, "Right Trig"}
};

void Joystick::update(double delta_time) {
    GLFWgamepadstate p;
    glfwGetGamepadState(0, &p);

    if (!mtx_reentrancy.try_lock())
        return;

    for (auto &joy : joysticks)
        joy.second.update();

    process_input(delta_time);
    set_robot(delta_time);

    mtx_reentrancy.unlock();
}

void Joystick::process_input(double delta_time) {
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
            auto sp = vec3_d(dir) * vec3_d(0.01) * delta_time;
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

void Joystick::query_joysticks() {
    map_type joys = map_type();
    static bool first_run = true;

    for (int jid = GLFW_JOYSTICK_1; jid < GLFW_JOYSTICK_LAST; jid++) {
        if (!glfwJoystickPresent(jid))
            continue;

        // Note, GUID can match two discrete joysticks from a single device

        auto kv = JoystickDevice::get_device(jid);
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

void Joystick::reset() {

}

void Joystick::remove(const std::string &guid) {
    fprintf(stderr, "Remove joystick %s\n", guid.c_str());
    joysticks.erase(guid);
}

void Joystick::set_robot(double delta_time) {
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
            camera->joystick_move(window, pitch * 2 * delta_time, yaw * 2 * delta_time);


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
            
            camera->position += vvv * delta_time;
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
            ) * delta_time;
            robot_target += vvv;

            if (glm::length2(vvv) > 0)
                kinematics->solve_inverse(robot_target);

            vec3_d pd(axes[2] * 1.5, (axes[4] + 1) / 2, (axes[5] + 1) / 2), ndd(0.0);
            set_v3(ndd, pd, tolerance);
            ndd = scale_axes(ndd) * delta_time;

            if (glm::length2(ndd) > 0) {
                float s2r = s2->get_rotation(false);
                float s1r = s1->get_rotation(false);

                s2r += ndd.x * delta_time;
                s1r -= ndd.y * delta_time;
                s1r += ndd.z * delta_time;

                s2r = util::clip(s2r, -120, 120); // because we can't check robot_interface unless refactor, works for now
                s1r = util::clip(s1r, -50, 50);

                s2->set_rotation(s2r);
                s1->set_rotation(s1r);
            }
        }
    }
}

void Joystick::query_robot() {
    robot_interface->read_all();
    for (auto *seg : servo_segments)
        fprintf(stderr, "%i: %i (%.2f deg), ", seg->servo_num, seg->servo_cur_position, seg->to_degrees(seg->servo_cur_position));
    fputs("\n", stderr);
}

void Joystick::rest_robot() {
    robot_interface->servos_off();
}

void Joystick::rest_position_robot() {
    auto rest = [&]() {
        float iv = 1000.0;
        robot_interface->set_servos({
            {1, 500},
            {2, 500},
            {3, 500},
            {4, 500},
            {5, 500},
            {6, 500}
        }, iv);

        std::this_thread::sleep_for(dur(iv));

        robot_interface->set_servo(3, 1, iv);
    };

    auto thread = std::thread(rest);
}

void Joystick::connect_robot() {
    robot_interface->open();
    set_segments_from_robot();
}

std::string Joystick::get_debug_info() {
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
    }   
    return info;
}