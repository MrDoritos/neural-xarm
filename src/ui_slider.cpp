#include "util.h"
#include "ui_slider.h"

ui_slider_t::ui_slider_t(GLFWwindow *window, shader_program_t *textProgram, texture_t *textTexture, glm::vec4 XYWH, ui_slider_v min, ui_slider_v max, ui_slider_v value, std::string title, bool limit, callback_t value_change_callback, bool skip_text, bool hidden):
        ui_element_t(window, XYWH),
        min(min),
        max(max),
        value(value),
        initial_value(value),
        cursor_drag(false),
        limit(limit),
        skip_text(skip_text),
        title_cached(title),
        textProgram(textProgram),
        textTexture(textTexture),
        title_text(0),
        min_text(0),
        max_text(0),
        value_text(0),
        text_pos_y(0),
        title_pos_y(0) {
    this->hidden = hidden;

    if (!skip_text) {
        if (title.size() > 0) {
            title_text = add_child(new ui_text_t(window, textProgram, textTexture, XYWH));
        }
        min_text = add_child(new ui_text_t(window, textProgram, textTexture, XYWH));
        max_text = add_child(new ui_text_t(window, textProgram, textTexture, XYWH));
        value_text = add_child(new ui_text_t(window, textProgram, textTexture, XYWH));
    }

    arrange_text();

    this->value_change_callback = value_change_callback;
}

void ui_slider_t::set_min(ui_slider_v min) {
    this->min = min;

    if (min_text) {
        min_text->XYWH = XYWH + glm::vec4(-value_subpos_x,text_pos_y,0,0);
        min_text->set_string(vtos(min));
    }

    modified = true;
}

void ui_slider_t::set_max(ui_slider_v max) {
    this->max = max;

    if (max_text) {
        max_text->XYWH = XYWH + glm::vec4(XYWH[2]-value_subpos_x,text_pos_y,0,0);
        max_text->set_string(vtos(max));
    }

    modified = true;
}

std::string ui_slider_t::vtos(ui_slider_v v) {
    const int buflen = 100;
    char buf[buflen];
    snprintf(buf, buflen, "%.1f", v);
    return std::string(buf);
}

void ui_slider_t::set_value(ui_slider_v v, bool callback) {
    ui_slider_v _vset = v;

    if (limit)
        _vset = util::clip(_vset, min, max);
        
    this->value = _vset;
    if (value_text) {
        auto slider = get_slider_position();
        value_text->XYWH = glm::vec4(slider.x, text_pos_y + XYWH.y - value_subpos_y, 0,0);
        value_text->set_string(vtos(value));
    }

    if (value_change_callback && callback)
        value_change_callback(this, this->value);

    modified = true;
}

void ui_slider_t::set_title(std::string title) {
    if (title_text) {
        title_pos_y = -title_height;
        title_text->XYWH = XYWH + glm::vec4(0,title_pos_y,0,title_height);
        if (title.size() > 0)
            title_cached = title;
        title_text->set_string(title_cached);
    }
}

bool ui_slider_t::arrange_text() {
    if (skip_text) {
        return glsuccess;
    }

    if (title_cached.size() > 0) {
        set_title();
        text_pos_y = title_pos_y + title_height - value_subpos_y;
    } else {
        text_pos_y = -value_subpos_y;
    }

    set_min(min);
    set_max(max);
    set_value(value);

    return glsuccess;
}

bool ui_slider_t::reset() {
    //Resets children text
    ui_element_t::reset();

    if (skip_text)
        return glsuccess;

    //Sets the text after a reset
    value = initial_value;
    arrange_text();

    return glsuccess;
}

std::string ui_slider_t::get_element_name() {
    return "ui_slider_t";
}

bool ui_slider_t::onCursor(double x, double y) {
    if (hidden)
        return glsuccess;

    if (cursor_drag) {
        bool shift = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS);
        float fact = shift ? precise_factor : 1.;

        auto current_cursor = get_cursor_relative();
        ui_slider_v v_diff = (current_cursor.x - cursor_start.x);

        if (v_diff * v_diff < granularity * granularity) 
            return glcaught; //don't update slider when difference is negligible

        cursor_start = current_cursor;

        ui_slider_v v_raw = v_diff / (XYWH[2] * fact);
        ui_slider_v v_map = (v_raw * (max - min)) + min;
        ui_slider_v v_range = (max - min);
        //printf("set slider <%f,%f>\n", v_raw, v_map);
        //set_value( v_map);
        set_value(v_range * v_raw + value);

        return glcaught;
    }
    return ui_element_t::onCursor(x,y);
}

glm::vec4 ui_slider_t::get_slider_position() {
    ui_slider_v bound = std::min(std::max(value, min), max);
    ui_slider_v min = std::min(this->max, this->min);
    ui_slider_v max = std::max(this->max, this->min);

    auto in = XYWH;
    ui_slider_v norm = ((bound - min) / (max - min));

    auto mp = get_midpoint_relative();
    auto size = get_size();
    auto pos = get_position();
    auto i_size = mp * glm::vec2(0.5);
    auto half_i_size = i_size / glm::vec2(2.);
    
    //return glm::vec4(norm * size.x + pos.x - half_i_size.x, mp.y - half_i_size.y, i_size.x, i_size.y);

    auto _pos = (norm * in[2]) + in[0];
    auto itemh = in[3] * slider_frac_h;
    //auto itemw = in[2] * slider_frac_w;
    auto itemw = itemh * slider_frac_w;
    auto itemwh = glm::vec2(itemw, itemh); //.1

    auto mp_y = (in[3] / 2.) + in[1];
    auto itemhf = itemwh / glm::vec2(2.);

    //printf("norm: %f, pos: %f, itemh: %f, mp_y: %f, value: %f, min: %f, max: %f, bound: %f, min: %f, max: %f\n", norm, pos, itemh, mp_y, value, min, max, bound, this->min, this->max);

    return glm::vec4(_pos - itemhf.x, mp_y - itemhf.y, itemwh.x, itemwh.y);
}

bool ui_slider_t::onMouse(int button, int action, int mods) {
    if (hidden)
        return glsuccess;

    if (cursor_drag && button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        cursor_drag = false;
        set_value(drag_value);
        return glcaught;
    }

    if (is_cursor_bound() && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        cursor_drag = true;
        cursor_start = get_cursor_relative();
        drag_start = cursor_start;
        drag_value = value;
        return glcaught;
    } else {
        cursor_drag = false;
    }
    return ui_element_t::onMouse(button, action, mods);
}

bool ui_slider_t::mesh() {
    modified = false;
    vertexCount = 0;

    text_t *buffer = new text_t[1 * 18];

    auto mp = get_midpoint_relative();
    auto pos = get_position();
    auto size = get_size();

    glm::vec4 slider_bar_position = get_slider_position();
    float slider_range_height = slider_bar_position[3] / 4.;
    glm::vec4 slider_range_position = {pos.x,pos.y+mp.y-(slider_range_height/2),size.x,slider_range_height};

    glm::vec4 slider_range_color(0,0,0,.75); //75% alpha black
    glm::vec4 slider_bar_color(0,0,.5,1); //50% blue

    //background
    //add_rect(buffer, vertexCount, XYWH, slider_range_color, true);
    //slider range
    add_rect(buffer, vertexCount, slider_range_position, slider_range_color, true);
    //slider
    add_rect(buffer, vertexCount, slider_bar_position, slider_bar_color, true);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof * buffer, buffer, GL_STATIC_DRAW);

    buffer->set_attrib_pointers();

    delete [] buffer;

    return glsuccess;
}

bool ui_slider_t::render() {
    if (modified)
        mesh();

    if (vertexCount < 1 || hidden)
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