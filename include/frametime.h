#pragma once

#include "common.h"

namespace gui {

template<typename T = double, typename CLK = hrc>
struct frametime_T {
    using DUR = std::chrono::duration<T>;
    using TP = std::chrono::time_point<CLK>;
    using value_type = T;

    frametime_T():
    fps(0),
    T_dt(0),
    frame_count(0),
    tp_start(CLK::now()),
    tp_end(CLK::now()),
    tp_ft(CLK::now()),
    dur_ft(0),
    dur_dt(0) {

    }

    T fps, T_dt;
    int frame_count;
    DUR dur_ft, dur_dt;
    TP tp_start, tp_end, tp_ft;

    template<typename R>
    inline R get_delta_time();

    inline T get_fps() {
        return fps;
    }

    inline T get_ms() {
        return get_delta_time<T>() * 1000.0;
    }

    // Call once per frame
    void update() {
        frame_count++;

        tp_start = CLK::now();

        dur_dt = tp_start - tp_end;
        T_dt = dur_dt.count();
        tp_end = tp_start;

        dur_ft = tp_start - tp_ft;
        T T_ft = dur_ft.count();

        if (T_ft >= 1.0) {
            fps = frame_count / T_ft;
            frame_count = 0;
            tp_ft = tp_start;
        }
    }
};

template<>
template<>
inline frametime_T<>::DUR frametime_T<>::get_delta_time<frametime_T<>::DUR>() {
    return dur_dt;
}

template<>
template<>
inline frametime_T<>::value_type frametime_T<>::get_delta_time<frametime_T<>::value_type>() {
    return T_dt;
}

using frametime_t = frametime_T<>;

}