#include "segment.h"
#include "debug_object.h"
#include "xarm_common.h"

namespace robot {
 
float d_unit = 1.0f / 10.0f;
float gravity = 9.81f;

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
    auto force_v = glm::vec3(0, mass * gravity, 0);

    return force_v;
}

template<>
glm::vec3 Segment::get_total_force(const bool &allow_interpolate) const {    
    auto sf = get_self_force(allow_interpolate);

    if (child)
        return sf + child->get_total_force(allow_interpolate);
    
    return sf;
}

template<>
std::pair<glm::vec3, glm::vec3> Segment::get_self_center_force(const bool &allow_interpolate) const {
    auto f = get_self_force(allow_interpolate);
    auto m = get_midpoint(allow_interpolate);

    return {m, f};
}

template<>
std::pair<glm::vec3, glm::vec3> Segment::get_total_center_force(const bool &allow_interpolate) const {
    auto cf = get_self_center_force(allow_interpolate);

    if (child) {
        auto c_cf = child->get_total_center_force(allow_interpolate);
        return {cf.first + c_cf.first, cf.second + c_cf.second};
    }

    return cf;
}

template<>
glm::vec3 Segment::get_self_torque(const bool &allow_interpolate, const glm::vec3 &origin) const {
    auto [mp, f] = get_self_center_force(allow_interpolate);

    glm::vec2 xz_self(mp.x, mp.z), xz_origin(origin.x, origin.z);
    //auto y_torque = glm::distance(xz_self, xz_origin) * f.y;
    auto y_fact = glm::normalize(origin + mp);
    //auto y_torque = glm::dot(xz_origin, xz_origin + xz_self) * f.y;
    auto y_torque = (fabs(y_fact.x) + fabs(y_fact.z)) * f.y;

    //return (origin + mp) * f;
    return glm::vec3(0, y_torque, 0);
}

template<>
glm::vec3 Segment::get_total_torque(const bool &allow_interpolate, const glm::vec3 &origin) const {
    auto st = get_self_torque(allow_interpolate, origin);
    auto so = get_origin(allow_interpolate);

    if (child)
        return st + child->get_total_torque(allow_interpolate, so);

    return st;
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
    //float load = get_total_force(allow_interpolate).y / torque;
    auto so = get_origin(allow_interpolate);
    float load = get_total_torque(allow_interpolate, so).y / torque;

    auto origin = get_origin(allow_interpolate);
    auto mp = origin + get_midpoint(allow_interpolate);
    debug_objects->add_line(mp, mp - (glm::vec3(0,1,0) * load * 2.0f));

    return load;
}

}