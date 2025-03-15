#include "text.h"

float title_height = 0.0483;
float value_subpos_x = 0.06167;
float value_subpos_y = 0.0117;
float slider_frac_w = 1.;
float slider_frac_h = 0.5;
float granularity = 0.005;
float precise_factor = 20.;
text_parameters global_text_parameters;

text_parameters::text_parameters() {
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

text_parameters::text_parameters(const int &count, const float &scale, const float &spacing, const float &padding, const float &margin)
    :text_parameters() {
    texCharacterCountX = count;
    texCharacterCountY = count;

    scrScale = scale;
    scrCharacterSpacingScale = spacing;
    texCharacterTrim = padding;
    scrCharacterTrim = margin;
}

void text_parameters::calculate(const int character, const int currentX, const int currentY, const v4 SCR_XYWH, v4 &CHAR_XYWH, v4 &UV_XYWH) const {
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

void text_t::set_attrib_pointers() {
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof * this, (const void*) (0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof * this, (const void*) (8));
    glEnableVertexAttribArray(1);
}

void add_rect(text_t *buffer, unsigned int &vertexCount, const glm::vec4 &XYWH, const glm::vec4 &UVWH, const bool &is_color) {
    const float &x = XYWH.x, &y = XYWH.y, &w = XYWH.z, &h = XYWH.w;
    const float &u = UVWH.x, &v = UVWH.y, &uw = UVWH.z, &vh = UVWH.w;

    const float verticies[6][6] = {
        {x,y,u,v,0,0},
        {x+w,y,u+uw,v,0,0},
        {x,y+h,u,v+vh,0,0},
        {x+w,y,u+uw,v,0,0},
        {x+w,y+h,u+uw,v+vh,0,0},
        {x,y+h,u,v+vh,0,0}
    };

    /*
    for (int i = 0; i < sizeof verticies / sizeof verticies[0]; i++) {
        memcpy(&buffer[vertexCount], &verticies[i], sizeof verticies[0]);

        if (is_color)
            memcpy(buffer[vertexCount].tex_coords_ptr(), &UVWH, sizeof UVWH);

        vertexCount++;
    }
    */
   
    memcpy(&buffer[vertexCount], &verticies[0], sizeof verticies);

    if (is_color) {
        for (int i = 0; i < sizeof verticies / sizeof verticies[0]; i++)
            memcpy(buffer[vertexCount + i].tex_coords_ptr(), &UVWH, sizeof UVWH);
    }

    vertexCount += 6;
}