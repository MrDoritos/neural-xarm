#pragma once

#include "common.h"
#include "xarm_common.h"
#include "robot_interface.h"
#include "camera.h"
#include "kinematics.h"
#include <mutex>

namespace {

struct JoystickDevice {
    inline JoystickDevice(int jid)
        :JoystickDevice(get_device(jid).second) { }

    inline JoystickDevice() { }

    int axis_count, button_count, jid;
    std::string guid, name, gp_name;
    std::map<int, bool> held_buttons;
    std::vector<float> deadzones;
    GLFWgamepadstate state;

    inline int get_button(int button);

    inline void set_deadzones();

    inline void update();

    static std::pair<std::string, JoystickDevice> get_device(int jid); 
};

struct Joystick {
    using map_type = std::map<std::string, JoystickDevice>;

    map_type joysticks;
    bool pedantic_debug, camera_move;
    std::mutex mtx_reentrancy;
    RobotInterface *robot_interface;
    camera_t *camera;
    Kinematics *kinematics;

    static std::map<int, std::string> button_mapping, axis_mapping;

    inline Joystick(RobotInterface *robot_interface, camera_t *camera, Kinematics *kinematics)
        :robot_interface(robot_interface),
         camera(camera),
         kinematics(kinematics) { }

    template<typename T, int c = 3, typename vec = glm::vec<c, T>>
    inline vec scale_axes(const vec &in) {
        vec ret(0.0);
        for (int i = 0; i < c; i++) {
            T sign = in[i] >= 0 ? 1 : -1;
            ret[i] = (0.0 + (1.0 * powf64(0.0 + abs(in[i]), 2))) * sign;
        }
        return ret;
    }

    void process_input(double delta_time);

    void query_joysticks();

    void update(double delta_time);

    void reset();

    void remove(const std::string &guid);

    void set_robot(double delta_time);

    void rest_robot();

    void rest_position_robot();

    void query_robot();

    void connect_robot();

    std::string debug_info();


};

}