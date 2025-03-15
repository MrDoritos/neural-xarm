#include "ui_toggle.h"

ui_toggle_t::ui_toggle_t(GLFWwindow *window, shader_program_t *textProgram, texture_t *textTexture, glm::vec4 XYWH, std::string title, bool initial_state, toggle_callback_t toggle_callback):
        ui_element_t(window, XYWH),
        toggle_state(initial_state),
        initial_state(initial_state),
        held(false),
        cached_title(title),
        toggle_callback(toggle_callback),
        textTexture(textTexture),
        textProgram(textProgram) {
    if (cached_title.size() > 0)
        title_text = add_child(new ui_text_t(window, textProgram, textTexture, XYWH - glm::vec4(title_height/4.,title_height,0,0), cached_title));
}

bool ui_toggle_t::onMouse(int key, int action, int mods) {
    if (is_cursor_bound() && key == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_RELEASE) {
            set_toggle();
            set_held(false);
        } else
        if (action == GLFW_PRESS) {
            set_held(true);
        }
        return glcaught;
    }

    return glsuccess;
}

void ui_toggle_t::set_toggle() {
    set_state(!toggle_state);
}

void ui_toggle_t::set_held(bool state) {
    if (held != state) {
        modified = true;
        held = state;
    }
}

void ui_toggle_t::set_state(bool state) {
    modified = true;
    toggle_state = state;
    if (toggle_callback)
        toggle_callback(this, toggle_state);
}

bool ui_toggle_t::reset() {
    ui_element_t::reset();

    set_held(false);
    set_state(initial_state);
    if (title_text)
        title_text->set_string(cached_title);

    return glsuccess;
}

bool ui_toggle_t::mesh() {
    modified = false;
    vertexCount = 0;

    text_t *buffer = new text_t[2 * 6];

    glm::vec4 button_held_color(0,0,0.5,1.);
    glm::vec4 button_on_color(0,0,1.0,1.);
    glm::vec4 button_off_color(0,0,0.1,1.);

    glm::vec4 color = (held ? button_held_color : (toggle_state ? button_on_color : button_off_color));

    add_rect(buffer, vertexCount, XYWH, color, true);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof * buffer, buffer, GL_STATIC_DRAW);

    buffer->set_attrib_pointers();

    delete [] buffer;

    return glsuccess;
}

bool ui_toggle_t::render() {
    if (modified)
        mesh();

    if (vertexCount < 1)
        return glsuccess;

    assert(textProgram && "textProgram null\n");

    textProgram->use();
    textProgram->set_f("mixFactor", 1.0);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    glBindVertexArray(0);

    ui_element_t::render();

    return glsuccess;
}