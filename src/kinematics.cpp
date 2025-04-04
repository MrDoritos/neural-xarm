#include "kinematics.h"

namespace robot {

bool Kinematics::solve_inverse(vec3_d coordsIn) {
    using fp_t = vec3_d::value_type;
    
    using vec3 = glm::vec<3, fp_t>;
    using vec2 = glm::vec<2, fp_t>;

    /*
        Assume Y-up       
    */

    //std::vector<Segment*> &segments = visible_segments;
    std::vector<Segment*> segments(visible_segments.begin(), visible_segments.end()-2);

    bool calculation_failure = false;

    vec3 target_3d = coordsIn * vec3(1,1,-1);
    vec2 target_2d(target_3d.x, target_3d.z);

    std::vector<fp_t> segment_rotations;
    std::for_each(segments.begin(), segments.end(), [&segment_rotations](const Segment *seg) {
        segment_rotations.push_back(seg->get_clamped_rotation<fp_t>(false));
    });

    /*
        Solve for the rotation of the base

        We are not concerned with the up axis
    */
    vec3 segment_6_3d = s6->get_origin(false);
    vec2 segment_6_2d(segment_6_3d.x, segment_6_3d.z);

    vec2 magnitude_6 = glm::normalize(target_2d - segment_6_2d);
    fp_t segment_6_rad = atan2(magnitude_6.y, magnitude_6.x) - M_PI;

    /*
        segment_5_3d is passed as the translation for the map_to_xy

        target and segment 5 can be thought of as being shifted by the
        same amount, although eventually this translation is not a
        concern
    */
    vec3 segment_5_3d = s5->get_origin(false);

    /*
        Since all segments beyond the rotating base lie upon the same plane,
        we can treat the origin of the first servo after the rotating base
        as being on the same axis as the coordinate target, making
        calculation easier
    */
    vec3 target_pl3d = util::map_to_xy<fp_t>(target_3d, glm::degrees(segment_6_rad), vec3(y_axis), segment_5_3d);
    vec2 target_pl2d(target_pl3d.x, target_pl3d.y);

    /*
        Probably residual, but we are unlikely to need to translate the
        target relative to an existing segment, after the translation in
        map_to_xy
    */
    vec3 target_offset_3d(0);
    vec2 target_offset_2d(0);

    /*
        Take the segments that lie on the same plane for calculation
    */
    std::vector<Segment*> remaining_segments;
    remaining_segments.assign(segments.begin() + 2, segments.end());

    /*
        prev_origin is for the loop, all calculations rely on a
        previous origin
    */
    vec2 prev_origin = target_pl2d;
    /*
        calculated origins end up here. we do not need to calculate the servo angles yet
    */
    std::vector<vec2> new_origins;

    if (debug_pedantic)
        printf("target_pl2d <%.2f,%.2f> target_pl3d <%.2f,%.2f,%.2f> target_real <%.2f,%.2f,%.2f> seg_5 <%.2f,%.2f,%.2f> deg_6: %.2f\n", target_pl2d.x, target_pl2d.y, target_pl3d.x, target_pl3d.y, target_pl3d.z, target_3d.x, target_3d.y, target_3d.z, segment_5_3d.x, segment_5_3d.y, segment_5_3d.z, glm::degrees(segment_6_rad));

    while (true) {
        if (remaining_segments.size() < 1) {
            if (debug_pedantic)
                puts("No more segments");
            break;
        }

        Segment *seg = remaining_segments.back();
        fp_t segment_radius = seg->get_length_temp();
        
        /*
            The total_length is the total length of segments remaining
            that can still have their origin altered. It includes the
            current segment_radius
        */
        fp_t total_length = 0.0;
        for (Segment *x : remaining_segments)
            total_length += x->get_length_temp();

        /*
            dist_to_segment is the absolute max the rest of the segments
            can accommodate
        */
        fp_t dist_to_segment = total_length - segment_radius;
        /*
            How far the previously calculated origin is to the target
            coordinate
        */
        fp_t dist_origin_to_prev = glm::distance(target_offset_2d, prev_origin);
        /*
            Used later to find the direction between the previous origin
            and the target point
        */
        vec2 mag = glm::normalize(prev_origin - target_offset_2d);
        
        seg->debug_color = {0.,1.,0};

        if (remaining_segments.size() < 3 && debug_pedantic)
            puts("2 or less segments left");

        /*
            Specifically relating to the radical line,
            if I recall

            https://en.wikipedia.org/wiki/Radical_axis#Properties
        */
        fp_t equal_mp = ((dist_origin_to_prev * dist_origin_to_prev) -
                    (segment_radius * segment_radius) +
                    (dist_to_segment * dist_to_segment)) /
                    (2 * dist_origin_to_prev);

        /*
            These are values from 0-2 relating to the
            intersection of two circles

            This is how I can sort of alter the behavior
            of the robot's movement
        */
        fp_t rem_dist = dist_origin_to_prev - equal_mp;
        fp_t rem_min = -segment_radius/2.0;
        fp_t rem_retract = 0.0;
        fp_t rem_extend = segment_radius * 0.5;
        fp_t rem_ex2 = rem_extend * 1.75;
        fp_t rem_max = segment_radius * 0.95;

        if (debug_pedantic)
            printf("servo: %i, rem_dist: %.2f, rem_max: %.2f, equal_mp: %.2f, segment_radius: %.2f, dist_origin_to_prev: %.2f, dist_to_segment: %.2f, total_length: %.2f, prev_origin <%.2f,%.2f>\n", seg->servo_num, rem_dist, rem_max, equal_mp, segment_radius, dist_origin_to_prev, dist_to_segment, total_length, prev_origin.x, prev_origin.y);

        vec2 new_origin = prev_origin;

        /*
            I don't exactly understand, other than I detect convergence
        */
        if (dist_to_segment < 0.05) {
            new_origins.push_back(new_origin);
            if (debug_pedantic)
                puts("Convergence");
            break;
        }

        /*
            I think this occurs also when it's out of bounds, not sure
        */
        if (rem_dist > rem_max) {
            if (debug_pedantic) {
                puts("Not enough overlap");
                printf("rem_dist: %.2f rem_max: %.2f\n", rem_dist, rem_max);
                seg->debug_color = {0.,0.,0.};
            }
        }

        /*
            If there is enough segments left, we can alter the intersection
            of the two circles. We can effectively change the angle of the servo
        */
        if (remaining_segments.size() > 2) {
            if (segment_radius > dist_origin_to_prev) {
                fp_t v = rem_extend - (segment_radius - dist_origin_to_prev);
                equal_mp = dist_origin_to_prev - v;
                if (debug_pedantic) {
                    seg->debug_color = {1.,0,0};
                    puts("Too close to origin");
                }
            } else
            if (rem_dist < rem_extend && total_length > dist_origin_to_prev) {
                equal_mp = dist_origin_to_prev - rem_extend;
                if (debug_pedantic) {
                    seg->debug_color = {1.,1,1};
                    puts("Maintain center of gravity");
                }
            } else
            if (rem_dist < rem_retract && total_length > dist_origin_to_prev) {
                equal_mp = dist_origin_to_prev - rem_retract;
                if (debug_pedantic) {
                    puts("Too much leftover length");
                    seg->debug_color = {1.,.5,.5};
                }
            } else
            if (rem_dist < rem_ex2 && rem_dist >= rem_extend && total_length > dist_origin_to_prev) {
                if (debug_pedantic) {
                    puts("Too much leftover length");
                    seg->debug_color = {0.,.5,.5};
                }
                fp_t r = rem_ex2 - rem_extend;
                r = (rem_dist - rem_extend) / r;
                fp_t v = r / 2.0;

                if (v > 0.4)
                    v -= (v - 0.38);

                equal_mp = dist_origin_to_prev - (v * segment_radius + rem_extend);
            }

            if (rem_dist < rem_min) {
                if (debug_pedantic)
                    puts("Too much overlap");
                    seg->debug_color = {0.25,0.25,0.25};
                rem_dist = rem_min;
            } else
            if (rem_dist > rem_max) {
                if (debug_pedantic)
                    puts("Not enough overlap");
                    seg->debug_color = {0,0,0.};
            } else {
                rem_dist = dist_origin_to_prev - equal_mp;
            }
        }

        if (debug_pedantic)
            printf("rem_dist: %.2f, rem_max: %.2f, equal_mp: %.2f, dist_origin_to_prev: %.2f, dist_to_segment: %.2f\n", rem_dist, rem_max, equal_mp, dist_origin_to_prev, dist_to_segment);

        /*
            Calculate the magnitude (rotation) with the new circle intersection
        */
        vec2 mp_vec = mag * equal_mp;
        fp_t n = sqrtf(fabs((segment_radius * segment_radius) - (rem_dist * rem_dist)));
        fp_t deg90 = (M_PI / 2.0);
        fp_t o = atan2(mag.y, mag.x) - deg90;
        vec2 new_mag = glm::normalize(vec2(cosf(o),sinf(o)));

        /*
            This is the new origin calculated with the rotation
        */
        new_origin = mp_vec + (new_mag * n);
        vec3 new_origin3d = vec3(new_origin.x, new_origin.y, 0.0);

        fp_t dist_new_prev = glm::distance(new_origin, prev_origin);

        if (debug_pedantic)
            printf("mp_vec <%.2f %.2f>, segment_radius: %.2f, rem_dist: %.2f, n: %.2f, o: %.2f, new_mag <%.2f,%.2f>, dist_new_prev: %.2f\n", mp_vec[0], mp_vec[1], segment_radius, rem_dist, n, o, new_mag[0], new_mag[1], dist_new_prev);

        if (calculation_failure)
            seg->debug_color = {1.0,0,0};

        fp_t tolerable_distance = 10.0;

        /*
            We can detect if our math sucks
        */
        if (fabs(dist_new_prev - segment_radius) > tolerable_distance) {
            if (debug_pedantic)
                puts("Distance to prev is too different");
            calculation_failure = true;
        }

        /*
            Too far and no segments remain
        */
        if (remaining_segments.size() < 1 && glm::distance(new_origin, target_pl2d) > tolerable_distance) {
            if (debug_pedantic)
                puts("Distance to target is too far");
            calculation_failure = true;
        }

        prev_origin = new_origin;
        new_origins.push_back(new_origin);
        remaining_segments.pop_back();
    }

    /*
        Usually triggered by the target being too far
    */
    if (calculation_failure) {
        if (debug_pedantic)
            puts("Failed to calculate");
        return glfail;
    }

    /*
        This probably doesn't happen, but just in case
    */
    if (new_origins.size() < 1) {
        if (debug_pedantic)
            puts("Not enough origins");
        return glfail;
    }

    fp_t prevrot = 0.0;
    vec2 prev(0.0);
    
    new_origins.pop_back();
    std::reverse(new_origins.begin(), new_origins.end());
    new_origins.push_back(target_pl2d);

    /*
        Calculating servo rotation from the origins is completed here

        i+2 because we start at the 3rd servo and iterate towards the
        6th servo
    */
    for (int i = 0; i < new_origins.size(); i++) {
        const vec2 &cur = new_origins[i];
        vec2 mag = glm::normalize(cur - prev); // direction

        fp_t rad = atan2(mag.x, mag.y) - prevrot; // angle

        if (util::is_not_real(rad))
            rad = glm::radians(segment_rotations[i + 2]); // we should get real numbers but in case we dont
        
        if (debug_pedantic)
            printf("servo: %i, rad: %.2f, prevrot: %.2f, cur[0]: %.2f, cur[1]: %.2f, prev[0]: %.2f, prev[1]: %.2f\n", i + 2, rad, prevrot, cur.x, cur.y, prev.x, prev.y);
        
        segment_rotations[i + 2] = rad;

        prev = cur;
        prevrot = prevrot + rad;
    }

    /*
        The algorithm is done by this point

        Now we set the program state
    */
    segment_rotations[1] = segment_6_rad;

    if (debug_pedantic)
        printf("End rot: %.2f,%.2f,%.2f,%.2f,%.2f\n", segment_rotations[0], segment_rotations[1], segment_rotations[2], segment_rotations[3], segment_rotations[4]);

    for (int i = 0; i < segments.size(); i++) {
        fp_t wrapped = util::wrap(glm::degrees(segment_rotations[i]), -180.0, 180.0);
        segments[i]->set_rotation_bound(wrapped);
    }

    set_sliders_from_segments();

    return glsuccess;
}

}