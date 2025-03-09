#pragma once

#include "common.h"

struct ui_element_t {
    std::vector<ui_element_t*> children;
    GLFWwindow *window;
    glm::vec4 XYWH;
    GLuint vao, vbo, vertexCount;
    bool modified, loaded, hidden;
    using callback_t = std::function<void()>;
    callback_t pre_render_callback;

    ui_element_t()
    :modified(true),loaded(false),hidden(false),window(0) {}

    ui_element_t(const ui_element_t &ui)
    :ui_element_t() {
        *this = ui;
    }

    template<typename ...Ts>
    ui_element_t(GLFWwindow *window, glm::vec4 xywh, Ts ...children_)
    :ui_element_t() {
        this->children = {children_...};
        this->window = window;
        this->XYWH = xywh;
    }

    template<typename T>
    T* add_child(T *ui) {
        static_assert(std::is_base_of_v<ui_element_t, T> && "Not valid base");
        children.push_back(ui);

        return ui;
    }

    virtual bool redraw() {
        modified = true;

        return run_children(&ui_element_t::redraw);
    }

    virtual bool render() {
        if (hidden)
            return glsuccess;

        if (pre_render_callback)
            pre_render_callback();

        auto ret = run_children(&ui_element_t::render);

        return ret;
    }

    virtual bool mesh() {
        modified = false;
        vertexCount = 0;

        return glsuccess;
    }

    virtual bool set_hidden() {
        this->hidden = true;
        auto ret = run_children(&ui_element_t::set_hidden);

        return glsuccess;
    }

    virtual bool set_visible() {
        this->hidden = false;
        auto ret = run_children(&ui_element_t::set_visible);

        return glsuccess;
    }

    virtual glm::vec2 get_cursor_position() {
        double x, y;
        int width, height;
        glfwGetCursorPos(window, &x, &y);
        glfwGetFramebufferSize(window, &width, &height);
        auto unmapped = glm::vec2(x / width, y / height);
        return unmapped * glm::vec2(2.) - glm::vec2(1.);
    }

    virtual glm::vec2 get_cursor_relative() {
        return get_cursor_position() - get_position();
    }

    virtual glm::vec2 get_midpoint_position() {
        return get_midpoint_relative() + get_position(); 
    }

    virtual glm::vec2 get_midpoint_relative() {
        return get_size() / glm::vec2(2.);
    }

    virtual glm::vec2 get_size() {
        return glm::vec2(XYWH[2], XYWH[3]);
    }

    virtual glm::vec2 get_position() {
        return glm::vec2(XYWH);
    }

    virtual bool is_cursor_bound() {
        auto cursor_pos = get_cursor_position();
        return (cursor_pos.x >= XYWH.x && cursor_pos.y >= XYWH.y &&
                cursor_pos.x <= XYWH.x + XYWH[2] && cursor_pos.y <= XYWH.y + XYWH[3]);
    }

    virtual std::string get_element_name() {
        return "ui_element_t";
    }

    virtual bool load() {
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

    virtual bool reset() { 
        return run_children(&ui_element_t::reset);
    }

    template<typename... Ts>
    using _func=bool(ui_element_t*, Ts...);

    template<typename Tfunc, typename... Ts>
    bool run_children(Tfunc func, Ts... values) {
        for (auto *element : children)
            if ((element->*func)(values...))
                return glcaught;
        return glsuccess;
    }

    virtual bool onKeyboard(float deltaTime) {
        return run_children(&ui_element_t::onKeyboard, deltaTime);
    }

    virtual bool onMouse(int button, int action, int mods) {
        return run_children(&ui_element_t::onMouse, button, action, mods);
    }

    virtual bool onFramebuffer(int width, int height) { 
        return run_children(&ui_element_t::onFramebuffer, width, height);
    }

    virtual bool onCursor(double x, double y) {
        return run_children(&ui_element_t::onCursor, x, y);
    }
};