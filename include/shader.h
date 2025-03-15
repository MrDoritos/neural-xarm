#pragma once

#include <fstream>
#include <sstream>

#include "common.h"

struct shader_t {
    GLuint shaderId;
    GLenum type;

    inline shader_t(const GLenum &type)
    :type(type),shaderId(gluninitialized) { }

    inline constexpr bool isLoaded() const {
        return shaderId != gluninitialized;
    }

    bool load(const std::string &path);
};