#pragma once

#include "common.h"
#include "util.h"

struct mesh_t;

template<typename SERVO_T = int, typename T = float>
struct robot_servo_T {
    inline robot_servo_T() {}

    inline robot_servo_T(int servo_num, 
                  SERVO_T servo_min, 
                  SERVO_T servo_max, 
                  SERVO_T servo_home, 
                  SERVO_T end_pos, 
                  SERVO_T cur_pos, 
                  T degrees_per_second, 
                  T steps_per_degree):
    servo_num(servo_num),
    servo_min(servo_min),
    servo_max(servo_max),
    servo_home(servo_home),
    servo_end_position(end_pos),
    servo_cur_position(cur_pos),
    degrees_per_second(degrees_per_second),
    steps_per_degree(steps_per_degree) {

    }

    using dur_type = long;
    using servo_type = SERVO_T;
    using value_type = T;
    using clk = std::chrono::high_resolution_clock;
    using tp = std::chrono::time_point<clk>;
    using dur = std::chrono::duration<dur_type, std::milli>;

    SERVO_T servo_cur_position, servo_end_position;
    SERVO_T servo_min, servo_max, servo_home;
    int servo_num;

    T degrees_per_second, steps_per_degree;
    tp last_command;

    dur_type min_command_interval = 20;
    SERVO_T min_command_threshold = 1;

    template<typename RET = T, typename VT = SERVO_T>
    inline constexpr RET to_degrees(const VT &v) const {
        return RET(v * steps_per_degree);
    }

    template<typename RET = SERVO_T, typename VT = T>
    inline constexpr RET to_servo(const VT &v) const {
        return RET(v * (T(1.0) / steps_per_degree));
    }

    template<typename RET = dur_type>
    inline constexpr RET get_elapsed_time() const {
        using _dur = std::chrono::duration<RET>;
        return (RET)std::chrono::duration_cast<_dur>(clk::now() - last_command).count();
    }
    
    inline constexpr bool movement_complete() const {
        return get_servo_interpolated() <= min_command_threshold;
    }

    inline constexpr bool ready_for_command() const {
        return get_elapsed_time() >= min_command_interval;
    }

    inline void set_servo(const SERVO_T &v) {
        servo_cur_position = get_servo_interpolated();
        last_command = clk::now();
        //servo_cur_position = servo_end_position;
        servo_end_position = v;
    }

    template<typename VT = T>
    inline void set_servo_degrees(const VT &v) {
        set_servo(get_servo(v));
    }

    inline constexpr SERVO_T get_servo() const {
        return servo_end_position;
    }

    template<typename VT>
    inline constexpr SERVO_T get_servo(const VT &degrees) const {
        return to_servo(degrees) + servo_home;
    }

    template<typename RET = T>
    inline constexpr RET get_servo_degrees() const {
        return to_degrees<RET>(get_servo() - servo_home);
    }

    template<typename RET = T, typename VT = SERVO_T>
    inline constexpr RET get_servo_degrees(const VT &v) const {
        return to_degrees<RET, VT>(v - servo_home);
    }

    template<typename RET = SERVO_T>
    inline constexpr RET get_servo_interpolated() const {
        RET d = servo_end_position - servo_cur_position;

        if (abs(d) < min_command_threshold)
            return servo_end_position;

        auto t = get_elapsed_time<float>();

        RET md = t * degrees_per_second;
        RET dir = d > 0 ? 1 : -1;
        RET intp = dir * md;

        if (abs(intp) > abs(d))
            return servo_end_position;

        return intp + servo_cur_position;
    }
    
    template<typename RET = T>
    inline constexpr RET get_servo_interpolated_degrees() const {
        return get_servo_degrees<RET, RET>(get_servo_interpolated<RET>());
    }

    inline constexpr SERVO_T get_servo_start() const {
        return servo_cur_position;
    }
};

using robot_servo_t = robot_servo_T<int, float>;

template<typename mesh_base = mesh_t/*, typename robot_servo_type_T = robot_servo_T<int, float>*/>
struct segment_T : public robot_servo_T<int, float> {
    using robot_servo_type = robot_servo_T<int, float>;
    segment_T() {}

    inline segment_T(segment_T *parent, mesh_base *mesh, const robot_servo_type &servo_config, const glm::vec3 &rotation_axis, const float &length)
    :robot_servo_type(servo_config),parent(parent),mesh(mesh),rotation_axis(rotation_axis),length(length) { }

    inline constexpr float get_clamped_rotation(const bool &allow_interpolate = false) const {
        return util::wrap(get_rotation(allow_interpolate), -180, 180);
    }

    inline void set_rotation_bound(const float &degrees) {
        //fprintf(stderr, "%i %f %i %i %f %f %i %f\n", servo_num, degrees, servo_min, servo_max, get_servo_degrees(servo_min), get_servo_degrees(servo_max), servo_home, get_servo_degrees(servo_home));
        auto min = get_servo_degrees(servo_min);
        auto max = get_servo_degrees(servo_max);
        if (min > max)
            std::swap(min, max);
        set_servo_degrees(util::clip(degrees, min, max));
    }

    inline void set_rotation(const float &degrees) {
        set_servo_degrees(degrees);
    }

    inline constexpr float get_rotation(const bool &allow_interpolate = true) const {
        //fprintf(stderr, "%i %f %f\n", servo_num, get_servo_interpolated_degrees(), get_servo_degrees());
        if (allow_interpolate)
            return get_servo_interpolated_degrees();
        else
            return get_servo_degrees();
    }

    inline constexpr float get_length() const {
        return length * model_scale;
    }

    inline constexpr glm::mat4 get_rotation_matrix(const bool &allow_interpolate = true) const {
        if (!parent)
            return glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(x_axis));

        return glm::rotate(parent->get_rotation_matrix(allow_interpolate), glm::radians(get_rotation(allow_interpolate)), rotation_axis);
    }

    inline constexpr glm::mat4 get_model_transform(const bool &allow_interpolate = true) const {
        const glm::vec3 origin = get_origin(allow_interpolate);
        glm::mat4 matrix(1.);
        
        matrix = glm::translate(matrix, origin);
        matrix *= get_rotation_matrix(allow_interpolate);
        matrix = glm::scale(matrix, glm::vec3(model_scale));
        
        return matrix;        
    }

    inline constexpr glm::vec3 get_segment_vector(const bool &allow_interpolate = true) const {
        return util::matrix_to_vector(get_rotation_matrix(allow_interpolate)) * get_length();
    }

    inline constexpr glm::vec3 get_origin(const bool &allow_interpolate = true) const {
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

    float model_scale = 0.1;
    glm::vec3 rotation_axis;
    segment_T<> *parent;
    glm::vec3 debug_color;
    float length;
    mesh_base *mesh;
};

using segment_t = segment_T<>;