#pragma once

#include "common.h"
#include "util.h"

struct mesh_t;

template<typename mesh_base = mesh_t>
struct segment_T {
    segment_T() {}

    segment_T(segment_T *parent, mesh_base *mesh, glm::vec3 rotation_axis, float length, int servo_num) {
        this->parent = parent;
        this->rotation_axis = rotation_axis;
        this->length = length;
        this->servo_num = servo_num;
        this->rotation = 0.0f;
        this->mesh = mesh;
    }

    constexpr inline float get_clamped_rotation(const bool &allow_interpolate = false) const {
        return util::clamp(allow_interpolate ? get_rotation() : rotation, -180, 180);
    }

    inline float get_rotation(const bool &allow_interpolate = true) const;

    constexpr inline float get_length() const {
        return length * model_scale;
    }

    constexpr inline glm::mat4 get_rotation_matrix(const bool &allow_interpolate = true) const {
        if (!parent)
            return glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(x_axis));

        return glm::rotate(parent->get_rotation_matrix(allow_interpolate), glm::radians(get_rotation(allow_interpolate)), rotation_axis);
    }

    constexpr inline glm::mat4 get_model_transform(const bool &allow_interpolate = true) const {
        const glm::vec3 origin = get_origin();
        glm::mat4 matrix(1.);
        
        matrix = glm::translate(matrix, origin);
        matrix *= get_rotation_matrix(allow_interpolate);
        matrix = glm::scale(matrix, glm::vec3(model_scale));
        
        return matrix;        
    }

    constexpr inline glm::vec3 get_segment_vector(const bool &allow_interpolate = true) const {
        return util::matrix_to_vector(get_rotation_matrix(allow_interpolate)) * get_length();
    }

    constexpr inline glm::vec3 get_origin(const bool &allow_interpolate = true) const {
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
    float length, rotation;
    int servo_num;
    mesh_base *mesh;
};

template<>
inline float segment_T<>::get_rotation(const bool &allow_interpolate) const {
    /*
    if (model_interpolation && allow_interpolate) {
        auto &rb = robot_interface->robot_servos;
        if (rb.contains(servo_num)) {
            auto &rs = rb[servo_num];
            return rs.get_deg<float>(rs.get_interpolated_pos<float>());
        }
    }
    */
    return rotation;
}

using segment_t = segment_T<mesh_t>;