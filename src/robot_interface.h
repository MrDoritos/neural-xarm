#pragma once

#include "common.h"
#include "segment.h"
#include <hidapi/hidapi.h>

namespace {

struct RobotInterface {
    using durl = std::chrono::duration<long, std::milli>;

    hid_device *device;
    std::string serial_number;
    bool servo_sleep_on_destroy = true;
    bool virtual_output = false;
    unsigned short vendor_id, product_id;

    inline RobotInterface(unsigned short vendor_id, unsigned short product_id, const wchar_t *serial_number_w = nullptr, bool permit_virtual = false);

    inline ~RobotInterface();

    template<typename RB, 
             typename PERIOD_T = int,
             typename SERVO_T = RB::servo_type,
             typename VT_T = RB::value_type,
             typename RB_TP = RB::tp,
             typename PAIR_T = std::pair<SERVO_T,SERVO_T>>
        requires std::is_base_of_v<robot_servo_T<SERVO_T, VT_T>, RB>
    bool get_servo_commend(RB *servo, const PERIOD_T &r_period, const RB_TP &batch_time, const PAIR_T &cmd) {
        return false;
    }

    void update();

    void destroy();

    void reset();

    void init();

    void close();

    int open();

    std::string get_hid_error();

    std::string debug_info();

    void set_robot_defaults();

    void read_all(bool set_pos = false);

    void set_servo(int id, int position, int millis = 1000);

    void set_servos(const std::vector<std::pair<int,int>> &poses, const int time = 1000);

    void servos_off();
};

}