#pragma once

#include "common.h"
#include "xarm_common.h"

namespace robot {

struct Kinematics {
    bool solve_inverse(vec3_d coordsIn);
};

}