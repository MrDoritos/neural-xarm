#include "segment.h"
#include "debug_object.h"
#include "xarm_common.h"

namespace robot {
 
float d_unit = 1.0f / 10.0f;

/*
    G = 9.8m/s^2
    force = mass * G
    torque = radius * force * sin(theta)
    torque of segment = (length / 2) * force * sin(theta)
    force of segments = (sum force)
    displacement = end segment vector + end origin - start origin
    torque of segments = (sum displacement / 2) * (sum force)
*/

template<>
glm::vec3 Segment::get_self_force(const bool &allow_interpolate) const {
    auto s_v = get_segment_vector(allow_interpolate);
    auto s_n = glm::normalize(s_v);
    float horizontal_distance = (1.0-fabs(s_n[1])) * length;

    if (util::is_not_real(horizontal_distance))
        horizontal_distance = 0.0;

    auto force_v = glm::vec3(0, (horizontal_distance * d_unit * 0.5) * mass, 0);

    auto origin = get_origin(allow_interpolate);
    auto mp = origin + get_midpoint(allow_interpolate);
    //debug_objects->add_line(mp, mp - (force_v * 2.0f));

    return force_v;
}

template<>
glm::vec3 Segment::get_total_force(const bool &allow_interpolate, glm::vec3 position, float start_mass) const {
    auto origin = get_origin(allow_interpolate);
    auto s_v = get_segment_vector(allow_interpolate);
    auto s_n = glm::normalize(s_v);
    //float horizontal_distance = (1.0-fabs(s_n[1])) * length;

    //fprintf(stderr, "%i %f %f %f\n", servo_num, s_v.x, s_v.y, s_v.z);

    //if (util::is_not_real(horizontal_distance))
    //    horizontal_distance = 0.0;

    //glm::vec3 total_position = s_v + position;
    glm::vec3 total_position = get_self_force(allow_interpolate) + position;

    //float total_horizontal = start_length + horizontal_distance;
    float total_mass = start_mass + mass;

    if (child)
        return child->get_total_force(allow_interpolate, total_position, total_mass);
    
    total_position = glm::abs(total_position);

    //float dist = glm::length(total_position) * (1.0-fabs(total_position[1]));
    float dist = total_position.y;

    //auto force_v = glm::vec3(0, (total_horizontal * d_unit * 0.5) * total_mass, 0);
    //auto force_v = total_position * d_unit * 0.5f * total_mass;
    auto force_v = glm::vec3(0, dist * d_unit * 0.5f * total_mass, 0);

    fprintf(stderr, "%i %f %f %f %f %f %f\n", servo_num, total_position.x, total_position.y, total_position.z, force_v.x, force_v.y, force_v.z);

    return force_v;
}

template<>
float Segment::get_axis_load(const glm::vec3 &normalized_force, const bool &allow_interpolate) const {
    auto r_m = get_rotation_matrix(allow_interpolate);

    auto rot_axis = rotation_axis;
    //std::swap(rot_axis[0], rot_axis[2]);
    //auto r_v = glm::vec3(glm::vec4(rotation_axis, 1.0) * r_m);
    auto r_v = glm::normalize(glm::vec3(r_m[0]));
    //std::swap(r_v[0], r_v[1]);
    //std::swap(r_v[0], r_v[2]);
    //r_v.x *= -1;

    auto ori = get_origin(allow_interpolate);
    auto mp = ori + get_midpoint(allow_interpolate);
    //debug_objects->add_line(mp, mp - (normalized_force * glm::dot(r_v, normalized_force) * 2.0f));
    debug_objects->add_line(ori - (r_v * 0.5f), ori + (r_v * 0.5f));
    auto rr = r_v;
    //std::swap(rr[0], rr[1]);
    //if (fabs(glm::dot(rr, normalized_force)) < 0.1) return 0.0;
    //return glm::dot(r_v + normalized_force, normalized_force);
    return glm::dot(r_v, normalized_force);
}

template<>
float Segment::get_servo_load(const bool &allow_interpolate) const {
    float load = get_total_force(allow_interpolate).y / torque;

    auto origin = get_origin(allow_interpolate);
    auto mp = origin + get_midpoint(allow_interpolate);
    debug_objects->add_line(mp, mp - (glm::vec3(0,1,0) * load * 2.0f));

    return load;
}

}