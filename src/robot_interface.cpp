#include "robot_interface.h"
#include "xarm_common.h"
#include <unistd.h>

//namespace {

RobotInterface::RobotInterface(unsigned short vendor_id, unsigned short product_id, const wchar_t *serial_number_w, bool permit_virtual)
        :vendor_id(vendor_id),
         product_id(product_id),
         virtual_output(permit_virtual),
         device(nullptr)
          {
    init();
}

RobotInterface::RobotInterface(bool permit_virtual)
    :RobotInterface(0, 0, 0, permit_virtual) { }

RobotInterface::~RobotInterface() {
    destroy();
}

void RobotInterface::update() {
    if (!device && !virtual_output)
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

void RobotInterface::destroy() {
    if (device) {
        if (servo_sleep_on_destroy)
            servos_off();
        fprintf(stderr, "Close robot connection\n");
        close();
    }
    hid_exit();
}

void RobotInterface::reset() {
    open(vendor_id, product_id);
}

void RobotInterface::init() {
    hid_init();
}

void RobotInterface::close() {
    if (!device)
        return;
    hid_close(device);
    device = nullptr;
}

int RobotInterface::open() {
    return open(vendor_id, product_id);
}

int RobotInterface::open(unsigned short vendor_id, unsigned short product_id, const wchar_t *serial_number_w) {
    this->vendor_id = vendor_id;
    this->product_id = product_id;

    if (device)
        close();

    device = hid_open(vendor_id, product_id, serial_number_w);

    if (!device) {
        if (virtual_output) {
            fprintf(stderr, "No handle, create virtual output\n");
            set_robot_defaults();
            return glsuccess;
        }
        fprintf(stderr, "Not connected to robot, (nonfatal) reason: %s\n", get_hid_error().c_str());
        serial_number = "No USB";
        return glfail;
    }

    std::wstring wstr;
    if (!serial_number_w) {
        wchar_t wbuf[100];
        if (!hid_get_serial_number_string(device, &wbuf[0], 100))
            wstr = std::wstring(&wbuf[0]);
        else 
            wstr = L"No serial";
    } else {
        wstr = std::wstring(serial_number_w);
    }
    serial_number = get_char_string(wstr);

    hid_set_nonblocking(device, 0);
    fprintf(stderr, "Open robot connection\n");

    set_robot_defaults();
    read_all(true);
    set_segments_from_robot();

    return glsuccess;
}

void RobotInterface::set_robot_defaults() {
    if (!device && virtual_output)
        serial_number = "Virtual Robot";
}

void RobotInterface::read_all(bool set_pos) {
    if (!device)
        return;

    const unsigned char cmd[11] = {
        0x55, 0x55, 9, 21, 6, 1, 2, 3, 4, 5, 6
    };
    hid_write(device, &cmd[0], 11);

    unsigned char ret[100];
    int count = hid_read_timeout(device, &ret[0], 100, 2000);
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

void RobotInterface::set_servo(int id, int position, int millis) {
    set_servos({{id,position}}, millis);
}

void RobotInterface::set_servos(const std::vector<std::pair<int,int>> &poses, const int time) {
    if (!device)
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

    hid_write(device, &cmd[0], count);
}

void RobotInterface::servos_off() {
    if (!device)
        return;

    unsigned char cmd[11] = { 0x55, 0x55, 9, 20, 6, 1, 2, 3, 4, 5, 6 };
    hid_write(device, &cmd[0], 11);
    usleep(100000); //why doesn't the command work every time, im trying to fix it
}

std::string RobotInterface::get_char_string(const std::wstring &str) {
    return std::string(str.begin(), str.end());
}
 
std::string RobotInterface::get_hid_error() {
    return get_char_string(hid_error(0));
}

std::string RobotInterface::get_debug_info() {
    std::string ret;
    ret += std::format("USB: {}\n", serial_number);
    for (auto *seg : servo_segments)
        ret += std::format("  {}: {}\n", seg->servo_num, seg->servo_cur_position);
    return ret;
}

//}