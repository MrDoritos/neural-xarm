#pragma once

#include "ui_text.h"

struct ui_toggle_t : public ui_element_t {
    using toggle_callback_t = std::function<void(ui_toggle_t*,bool)>;
    toggle_callback_t toggle_callback;

    bool toggle_state, held, initial_state;
    std::string cached_title;
    ui_text_t *title_text;
    shader_program_t *textProgram;
    texture_t *textTexture;

    ui_toggle_t(GLFWwindow *window, shader_program_t *textProgram, texture_t *textTexture, glm::vec4 XYWH, std::string title = std::string(), bool initial_state = false, toggle_callback_t toggle_callback = toggle_callback_t());

    bool onMouse(int key, int action, int mods) override;

    void set_toggle();

    void set_held(bool state);

    void set_state(bool state);

    bool reset() override;

    bool mesh() override;

    bool render() override;
};