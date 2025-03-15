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

    virtual bool redraw();

    virtual bool render();

    virtual bool mesh();

    virtual bool set_hidden();

    virtual bool set_visible();

    virtual glm::vec2 get_cursor_position();

    virtual glm::vec2 get_cursor_relative();

    virtual glm::vec2 get_midpoint_position();

    virtual glm::vec2 get_midpoint_relative();

    virtual glm::vec2 get_size();

    virtual glm::vec2 get_position();

    virtual bool is_cursor_bound();

    virtual std::string get_element_name();

    virtual bool load();

    virtual bool reset();

    template<typename... Ts>
    using _func=bool(ui_element_t*, Ts...);

    template<typename Tfunc, typename... Ts>
    bool run_children(Tfunc func, Ts... values) {
        for (auto *element : children)
            if ((element->*func)(values...))
                return glcaught;
        return glsuccess;
    }

    virtual bool onKeyboard(float deltaTime);

    virtual bool onMouse(int button, int action, int mods);

    virtual bool onFramebuffer(int width, int height);

    virtual bool onCursor(double x, double y);
};