#include "ui_text.h"

ui_text_t::ui_text_t(ui_element_t ui):
        ui_element_t(ui) {
    reset();
    params = 0;
    last_used = 0;
}

ui_text_t::ui_text_t(GLFWwindow *window, shader_program_t *shader, texture_t *texture, glm::vec4 xywh, std::string text, callback_t pre_render_callback):
        ui_element_t(window, xywh) {
    reset();
    params = 0;
    set_string(text);
    this->pre_render_callback = pre_render_callback;
    this->shader = shader;
    this->texture = texture;
}

void ui_text_t::string_change() {
    modified = true;
}

void ui_text_t::set_string(const std::string &str) {
    string_buffer = str;
    string_change();
}

void ui_text_t::set_string(const char* str) {
    std::string s = str;
    set_string(s);
    string_change();
}

void ui_text_t::add_line(const std::string &str) {
    string_buffer += str + '\n';
    string_change();
}

void ui_text_t::add_string(const std::string &str) {
    string_buffer += str;
    string_change();
}

bool ui_text_t::reset() {
    ui_element_t::reset();

    string_buffer.clear();
    modified = true;
    last_used = nullptr;

    return glsuccess;
}

text_parameters *ui_text_t::get_parameters() {
    return params ? params : &global_text_parameters;
}

bool ui_text_t::parameters_changed() {
    return false;
}

bool ui_text_t::render() {
    if (parameters_changed()) {
        if (debug_pedantic)
            puts("Parameters changed");
        modified = true;
    }

    if (hidden)
        return glsuccess;

    if (modified)
        mesh();

    assert(shader && "Shader null\n");
    assert(texture && "Texture null\n");

    shader->use();
    shader->set_sampler("textureSampler", texture);

    if (pre_render_callback)
        pre_render_callback();

    //if (debug_pedantic)
    //    printf("%s %i verticies\n", get_element_name().c_str(), vertexCount);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    glBindVertexArray(0);

    return run_children(&ui_element_t::render);
}

std::string ui_text_t::get_element_name() {
    const std::string basic_name = "ui_text_t";

    if (string_buffer.size() < 1)
        return basic_name;
    
    return basic_name + " (" + string_buffer + ")";
}

bool ui_text_t::mesh() {
    currentX = currentY = vertexCount = 0;

    //puts(string_buffer.c_str());
    last_used = get_parameters();

    if (string_buffer.size() < 1) {
        modified = false;
        return glsuccess;
    }

    if (!modified)
        return glsuccess;

    text_t *buffer = new text_t[string_buffer.size() * 6];

    for (auto ch : string_buffer) {
        if (ch == '\n') {
            currentX = 0;
            currentY++;
            continue;
        }

        glm::vec4 scr, tex;
        
        get_parameters()->calculate(ch, currentX, currentY, XYWH, scr, tex);
        add_rect(buffer, vertexCount, scr, tex);

        currentX++;
    }

    modified = false;

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof * buffer, buffer, GL_STATIC_DRAW);

    buffer->set_attrib_pointers();

    if (debug_ui)
        printf("Text buffer: %i verticies\n", vertexCount);

    delete [] buffer;

    return glsuccess;
}