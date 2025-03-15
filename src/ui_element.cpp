#include "ui_element.h"

bool ui_element_t::redraw() {
    modified = true;

    return run_children(&ui_element_t::redraw);
}

bool ui_element_t::render() {
    if (hidden)
        return glsuccess;

    if (pre_render_callback)
        pre_render_callback();

    auto ret = run_children(&ui_element_t::render);

    return ret;
}

bool ui_element_t::mesh() {
    modified = false;
    vertexCount = 0;

    return glsuccess;
}

bool ui_element_t::set_hidden() {
    this->hidden = true;
    auto ret = run_children(&ui_element_t::set_hidden);

    return glsuccess;
}

bool ui_element_t::set_visible() {
    this->hidden = false;
    auto ret = run_children(&ui_element_t::set_visible);

    return glsuccess;
}

glm::vec2 ui_element_t::get_cursor_position() {
    double x, y;
    int width, height;
    glfwGetCursorPos(window, &x, &y);
    glfwGetFramebufferSize(window, &width, &height);
    auto unmapped = glm::vec2(x / width, y / height);
    return unmapped * glm::vec2(2.) - glm::vec2(1.);
}

glm::vec2 ui_element_t::get_cursor_relative() {
    return get_cursor_position() - get_position();
}

glm::vec2 ui_element_t::get_midpoint_position() {
    return get_midpoint_relative() + get_position(); 
}

glm::vec2 ui_element_t::get_midpoint_relative() {
    return get_size() / glm::vec2(2.);
}

glm::vec2 ui_element_t::get_size() {
    return glm::vec2(XYWH[2], XYWH[3]);
}

glm::vec2 ui_element_t::get_position() {
    return glm::vec2(XYWH);
}

bool ui_element_t::is_cursor_bound() {
    auto cursor_pos = get_cursor_position();
    return (cursor_pos.x >= XYWH.x && cursor_pos.y >= XYWH.y &&
            cursor_pos.x <= XYWH.x + XYWH[2] && cursor_pos.y <= XYWH.y + XYWH[3]);
}

std::string ui_element_t::get_element_name() {
    return "ui_element_t";
}

bool ui_element_t::load() {
    run_children(&ui_element_t::load);

    if (loaded) {
        if (debug_pedantic)
            puts("Attempt to load twice");
        return glfail;
    }

    glGenBuffers(1, &vbo);
    glGenVertexArrays(1, &vao);
    vertexCount = 0;
    modified = true;
    loaded = true;

    if (debug_pedantic)
        printf("Loaded <%s,%i,%i,%i,%i,<%f,%f,%f,%f>>\n",
            get_element_name().c_str(), vbo, vao, vertexCount,
            modified, XYWH[0], XYWH[1], XYWH[2], XYWH[3]);

    return glsuccess;
}

bool ui_element_t::reset() { 
    return run_children(&ui_element_t::reset);
}

bool ui_element_t::onKeyboard(float deltaTime) {
    return run_children(&ui_element_t::onKeyboard, deltaTime);
}

bool ui_element_t::onMouse(int button, int action, int mods) {
    return run_children(&ui_element_t::onMouse, button, action, mods);
}

bool ui_element_t::onFramebuffer(int width, int height) { 
    return run_children(&ui_element_t::onFramebuffer, width, height);
}

bool ui_element_t::onCursor(double x, double y) {
    return run_children(&ui_element_t::onCursor, x, y);
}