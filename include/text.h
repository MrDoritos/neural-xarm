#pragma once

#include "common.h"

float title_height = 0.0483;
float value_subpos_x = 0.06167;
float value_subpos_y = 0.0117;
float slider_frac_w = 1.;
float slider_frac_h = 0.5;
float granularity = 0.005;
float precise_factor = 20.;

struct text_parameters {
    using v4 = glm::vec4;

    text_parameters() {
        texCharacterCountX = 16; texCharacterCountY = 16;

        scrScale = 1.; scrScaleX = 1.; scrScaleY = 1.;
        scrCharacterSpacingScale = 2.; scrCharacterSpacingScaleX = .5; scrCharacterSpacingScaleY = .8;
        texCharacterTrim = 1.; texCharacterTrimX = 4.; texCharacterTrimY = 16.;
        scrCharacterTrim = 1.; scrCharacterTrimX = 0.; scrCharacterTrimY = 0.;

        scrCharacterTrimX = 0.0121;
        scrCharacterTrimY = 0.00570;
        texCharacterTrimX = 8.73;
        texCharacterTrimY = 15.7;
        scrCharacterSpacingScaleX = 0.212;
        scrCharacterSpacingScaleY = 0.424;
        scrScaleX = .7;
        scrScaleY = .7;
    }

    text_parameters(int count, float scale, float spacing, float padding, float margin)
    :text_parameters() {
        texCharacterCountX = count;
        texCharacterCountY = count;

        scrScale = scale;
        scrCharacterSpacingScale = spacing;
        texCharacterTrim = padding;
        scrCharacterTrim = margin;
    }

    friend bool operator==(const text_parameters & lhs, const text_parameters & rhs) {
        return std::addressof(lhs) == std::addressof(rhs);
    }

    friend bool operator!=(const text_parameters & lhs, const text_parameters & rhs) {
        return !(lhs == rhs);
    }

    int texCharacterCountX, texCharacterCountY;
    float scrScale, scrScaleX, scrScaleY,
          scrCharacterSpacingScale, scrCharacterSpacingScaleX, scrCharacterSpacingScaleY,
          texCharacterTrim, texCharacterTrimX, texCharacterTrimY,
          scrCharacterTrim, scrCharacterTrimX, scrCharacterTrimY;

    void calculate(int character, int currentX, int currentY, v4 SCR_XYWH, v4 &CHAR_XYWH, v4 &UV_XYWH) {
        //final scale coefficient
        const float _scrScaleX = scrScaleX * scrScale;
        const float _scrScaleY = scrScaleY * scrScale;

        //dimensions in uv space
        const float _texCharacterWidth = 1.0f / texCharacterCountX;
        const float _texCharacterHeight = 1.0f / texCharacterCountY;

        //spacing coefficient
        const float _scrCharacterSpacingScaleX = scrCharacterSpacingScaleX * scrCharacterSpacingScale;
        const float _scrCharacterSpacingScaleY = scrCharacterSpacingScaleY * scrCharacterSpacingScale;

        //initial dimensions in screen space
        const float _scrCharacterSpacingX = _texCharacterWidth * _scrScaleX;
        const float _scrCharacterSpacingY = _texCharacterHeight * _scrScaleY;

        //initial position given by element
        const float _elementOriginX = SCR_XYWH.x;
        const float _elementOriginY = SCR_XYWH.y;

        //current character xy to index uv
        const int _characterIndexX = (character % texCharacterCountX);
        const int _characterIndexY = (character / texCharacterCountX % texCharacterCountY);

        //trim uv
        const float _texCharacterTrimX = _texCharacterWidth/texCharacterTrimX/texCharacterTrim;//texCharacterWidth / 5.; //pad uv x
        const float _texCharacterTrimY = _texCharacterHeight/texCharacterTrimY/texCharacterTrim;//texCharacterHeight / 16.;

        //trim scr
        const float _scrCharacterTrimX = scrCharacterTrimX * scrCharacterTrim;//scrCharacterSpacingX / 10.;
        const float _scrCharacterTrimY = scrCharacterTrimY * scrCharacterTrim;//scrCharacterSpacingY / 10.;

        //uv origin x/y
        const float _texOriginX = _characterIndexX * _texCharacterWidth + _texCharacterTrimX;
        const float _texOriginY = _characterIndexY * _texCharacterHeight + _texCharacterTrimY;

        //final uv w/h
        const float _texWidth = _texCharacterWidth - _texCharacterTrimX;
        const float _texHeight = _texCharacterHeight - _texCharacterTrimY;

        //screen origin x/y
        const float _scrOriginX = (currentX * _scrCharacterSpacingX * _scrCharacterSpacingScaleX) + _elementOriginX + _scrCharacterTrimX;
        const float _scrOriginY = (currentY * _scrCharacterSpacingY * _scrCharacterSpacingScaleY) + _elementOriginY + _scrCharacterTrimY;

        //final screen w/h
        const float _scrWidth = _scrCharacterSpacingX - _scrCharacterTrimX;
        const float _scrHeight = _scrCharacterSpacingY - _scrCharacterTrimY;

        CHAR_XYWH = {_scrOriginX, _scrOriginY, _scrWidth, _scrHeight};
        UV_XYWH = {_texOriginX, _texOriginY, _texWidth, _texHeight};
    }
};

struct text_data {
    using vec6 = glm::vec<6, float>;
    union {
        struct {float x, y, u, v, b, a;};
        float data[6];
    };

    glm::vec2 &coords() { return *reinterpret_cast<glm::vec2*>(&data[0]); }
    glm::vec2 *coords_ptr() { return reinterpret_cast<glm::vec2*>(&data[0]); }
    glm::vec4 &tex_coords() { return *reinterpret_cast<glm::vec4*>(&data[2]); }
    glm::vec4 *tex_coords_ptr() { return reinterpret_cast<glm::vec4*>(&data[2]); }
    vec6 *all_coords_ptr() { return reinterpret_cast<vec6*>(&data[0]); }
    vec6 &all_coords() { return *reinterpret_cast<vec6*>(&data[0]); }
};

struct text_t : public text_data {
    template<typename ...Ts>
    void set_values(Ts... values) {
        int i = 0;
        auto i_list = std::initializer_list<float>({values...});
        for (auto v : i_list)
            data[i++] = v;
    }

    text_t(glm::vec2 coords, glm::vec4 texCoords) {
        this->coords() = coords;
        this->tex_coords() = texCoords;
    }

    text_t(float x, float y, float u, float v, float b = 0., float a = 0.)
    :text_t({x,y},{u,v,b,a}) { }

    text_t() { }

    void set_attrib_pointers() {
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof * this, (const void*) (0));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof * this, (const void*) (8));
        glEnableVertexAttribArray(1);
    }
};

void add_rect(text_t *buffer, unsigned int &vertexCount, glm::vec4 XYWH, glm::vec4 UVWH = glm::vec4(0,0,1,1.), bool is_color = false) {
    float x = XYWH[0], y = XYWH[1], w = XYWH[2], h = XYWH[3];
    float u = UVWH[0], v = UVWH[1], uw = UVWH[2], vh = UVWH[3];

    text_t verticies[6] = {
        {x,y,u,v},
        {x+w,y,u+uw,v},
        {x,y+h,u,v+vh},
        {x+w,y,u+uw,v},
        {x+w,y+h,u+uw,v+vh},
        {x,y+h,u,v+vh}
    };

    for (int i = 0; i < sizeof verticies / sizeof verticies[0]; i++) {
        memcpy(&buffer[vertexCount], &verticies[i], sizeof verticies[0]);

        if (is_color)
            memcpy(buffer[vertexCount].tex_coords_ptr(), &UVWH, sizeof UVWH);

        vertexCount++;
    }
}

text_parameters global_text_parameters;