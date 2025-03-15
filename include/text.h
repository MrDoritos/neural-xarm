#pragma once

#include <glm/gtc/type_ptr.hpp>
#include "common.h"

extern float title_height;
extern float value_subpos_x;
extern float value_subpos_y;
extern float slider_frac_w;
extern float slider_frac_h;
extern float granularity;
extern float precise_factor;
extern struct text_parameters global_text_parameters;

struct text_parameters {
    using v4 = glm::vec4;

    text_parameters();

    text_parameters(const int &count, const float &scale, const float &spacing, const float &padding, const float &margin);

    inline constexpr friend bool operator==(const text_parameters & lhs, const text_parameters & rhs) {
        return std::addressof(lhs) == std::addressof(rhs);
    }

    inline constexpr friend bool operator!=(const text_parameters & lhs, const text_parameters & rhs) {
        return !(lhs == rhs);
    }

    int texCharacterCountX, texCharacterCountY;
    float scrScale, scrScaleX, scrScaleY,
          scrCharacterSpacingScale, scrCharacterSpacingScaleX, scrCharacterSpacingScaleY,
          texCharacterTrim, texCharacterTrimX, texCharacterTrimY,
          scrCharacterTrim, scrCharacterTrimX, scrCharacterTrimY;

    void calculate(const int character, const int currentX, const int currentY, const v4 SCR_XYWH, v4 &CHAR_XYWH, v4 &UV_XYWH) const;
};

struct text_data {
    using vec6 = struct glm::vec<6, float>;

    inline text_data(const glm::vec2 &c, const glm::vec4 &t):
    x(c[0]),y(c[1]),u(t[0]),v(t[1]),b(t[2]),a(t[3]) { }
    
    inline text_data() { }
    
    inline text_data(const float &x, const float &y, const float &u, const float &v, const float &b, const float &a):
    x(x),y(y),u(u),v(v),b(b),a(a) { }

    union {
        struct {float x, y, u, v, b, a;};
        float data[6];
    };

    inline glm::vec2 &coords() { return *reinterpret_cast<glm::vec2*>(&data[0]); }
    inline glm::vec2 *coords_ptr() { return reinterpret_cast<glm::vec2*>(&data[0]); }
    inline glm::vec4 &tex_coords() { return *reinterpret_cast<glm::vec4*>(&data[2]); }
    inline glm::vec4 *tex_coords_ptr() { return reinterpret_cast<glm::vec4*>(&data[2]); }
    inline vec6 *all_coords_ptr() { return reinterpret_cast<vec6*>(&data[0]); }
    inline vec6 &all_coords() { return *reinterpret_cast<vec6*>(&data[0]); }
};

struct text_t : public text_data {
    template<typename ...Ts>
    void set_values(Ts... values) {
        int i = 0;
        auto i_list = std::initializer_list<float>({values...});
        for (auto v : i_list)
            data[i++] = v;
    }

    inline text_t(const glm::vec2 &coords, const glm::vec4 &texCoords):
    text_data(coords, texCoords) { }

    inline text_t(const float &x, const float &y, const float &u, const float &v, const float &b = 0., const float &a = 0.):
    text_data(x,y,u,v,b,a) { }

    text_t() { }

    void set_attrib_pointers();
};

extern void add_rect(text_t *buffer, unsigned int &vertexCount, const glm::vec4 &XYWH, const glm::vec4 &UVWH = glm::vec4(0,0,1,1.), const bool &is_color = false);