#pragma once

#include "common.h"

struct texture_t {
    GLuint textureId;

    inline texture_t() : textureId(gluninitialized) { }

    inline constexpr bool isLoaded() const {
        return textureId != gluninitialized;
    }

    inline void use(const int &unit) const {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, textureId);
    }

    inline void use() const {
        glBindTexture(GL_TEXTURE_2D, textureId);
    }

    bool generate(const glm::vec4 &rgba);

    bool load(const std::string &path);
};
