#include "robot.h"
#include "kinematics.h"

namespace robot {

template<>
Robot::Robot_T() { }

template<>
Robot::Robot_T(seg_vec &segments)
    :segments(&segments) { }

template<>
Robot::range_type<robot::None> Robot::get_segments() {
    return range_type<robot::None>(segments);
}

template<>
Robot::range_type<robot::Visible> Robot::get_visible_segments() {
    return range_type<robot::Visible>(segments);
}

template<>
Robot::range_type<robot::Slider> Robot::get_slider_segments() {
    return range_type<robot::Slider>(segments);
}

template<>
Robot::range_type<robot::Kinematic> Robot::get_kinematic_segments() {
    return range_type<robot::Kinematic>(segments);
}

template<>
glm::vec3 Robot::get_end_effector_pos(const bool &interpolate) {
    return segments->back()->get_end_position(interpolate);
}

template<>
void Robot::set_end_effector_pos(glm::vec3 position) {
    
}

}