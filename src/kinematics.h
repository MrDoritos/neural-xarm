#pragma once

#include "common.h"
#include "xarm_common.h"

namespace robot {

struct Kinematics {
    template<typename T = float>
    constexpr bool is_not_real(const T &v) {
        return std::isinf(v) || std::isnan(v);
    }

    bool solve_inverse(vec3_d coordsIn);
};

}