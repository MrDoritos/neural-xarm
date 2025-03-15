#pragma once

#include "common.h"
#include "ui_element.h"
#include "shader_program.h"
#include "text.h"

struct ui_text_t : public ui_element_t {
    std::string string_buffer;
    int currentX, currentY;

    text_parameters *params, *last_used;
    shader_program_t *shader;
    texture_t *texture;

    ui_text_t(ui_element_t ui);

    ui_text_t(GLFWwindow *window, shader_program_t *shader, texture_t *texture, glm::vec4 xywh, std::string text = "", callback_t pre_render_callback = callback_t());

    void string_change();

    void set_string(const std::string &str);

    void set_string(const char* str);

    void add_line(const std::string &str);

    void add_string(const std::string &str);

    virtual text_parameters *get_parameters();

    virtual bool parameters_changed();

    bool reset() override;

    bool render() override;

    std::string get_element_name() override;

    protected:
    bool mesh() override;
};