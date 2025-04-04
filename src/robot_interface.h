#pragma once

#include "common.h"
#include "segment.h"
#include <hidapi/hidapi.h>

namespace robot {

struct RobotInterface {
    using durl = std::chrono::duration<long, std::milli>;

    hid_device *device;
    std::string serial_number;
    bool servo_sleep_on_destroy = true;
    bool virtual_output = false;
    unsigned short vendor_id, product_id;

    RobotInterface(unsigned short vendor_id, unsigned short product_id, const wchar_t *serial_number_w = nullptr, bool permit_virtual = false);

    RobotInterface(bool permit_virtual = false); 

    ~RobotInterface();

    template<typename RB, 
             typename PERIOD_T = int,
             typename SERVO_T = RB::servo_type,
             typename VT_T = RB::value_type,
             typename RB_TP = RB::tp,
             typename PAIR_T = std::pair<SERVO_T,SERVO_T>>
        requires std::is_base_of_v<robot_servo_T<SERVO_T, VT_T>, RB>
    bool get_servo_command(RB *servo, const PERIOD_T &r_period, const RB_TP &batch_time, PAIR_T &cmd) {
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

        if (fabs(dist) < servo->min_command_threshold && fabs(dist) < 1) {
            servo->servo_end_position = targeti;
            servo->servo_cur_position = targeti;
            return false;
        }

        auto mv = (r_period/1000.0) * servo->degrees_per_second;
        auto mvdist = servo->servo_cur_position - intrp;
        auto accel = mvdist / mv;
        auto jerk = (mvdist * mv) / mv;
        auto rintrp = intrp;

        if (fabs(jerk) > mv / 2 && fabs(mvdist) > 0) {
            intrp += (mvdist * .5);
        } else
        if (fabs(dist) < mv / 2 && fabs(jerk) < mv / 4) {
            intrp = targeti;
            if (debug_pedantic)
                fprintf(stderr, "Force set targeti\n");
        }

        servo->servo_end_position = targeti;
        servo->servo_cur_position = intrp;

        if (debug_pedantic)
            fprintf(stderr, "Send constant time s%i (%i/initialp -> %i/rintrp (jerk comp %i/intrp)) (%i/mvdist) (%i/targeti %.2f/targetf : %.2f/initialpf) = %i/dist (%i r_period/ms %.2lf mv/intpersec) accel %.2f jerk %.2f\n", servo->servo_num, initialp, rintrp, intrp, mvdist, targeti, targetf, initialpf, dist, r_period, mv, accel, jerk);

        cmd = { servo->servo_num, (int)intrp };

        return true;
    }

    void update();

    void destroy();

    void reset();

    void init();

    void close();

    int open();

    int open(unsigned short vendor_id, unsigned short product_id, const wchar_t *serial_number_w = nullptr);

    std::string get_hid_error();

    std::string get_debug_info();

    static std::string get_char_string(const std::wstring &str);

    void set_robot_defaults();

    void read_all(bool set_pos = false);

    void set_servo(int id, int position, int millis = 1000);

    void set_servos(const std::vector<std::pair<int,int>> &poses, const int time = 1000);

    void servos_off();
};

}