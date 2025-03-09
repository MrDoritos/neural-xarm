#pragma once

#include "common.h"
#include "ui_element.h"
#include "shader_program.h"
#include "text.h"

struct ui_text_t : public ui_element_t {
    std::string string_buffer;
    int currentX, currentY;

    text_parameters *params, *last_used;
    shaderProgram_t *shader;
    texture_t *texture;

    void string_change() {
        modified = true;
    }

    void set_string(const std::string &str) {
        string_buffer = str;
        string_change();
    }

    void set_string(const char* str) {
        std::string s = str;
        set_string(s);
        string_change();
    }

    void add_line(const std::string &str) {
        string_buffer += str + '\n';
        string_change();
    }

    void add_string(const std::string &str) {
        string_buffer += str;
        string_change();
    }

    bool reset() override {
        ui_element_t::reset();

        string_buffer.clear();
        modified = true;
        last_used = nullptr;

        return glsuccess;
    }

    ui_text_t(ui_element_t ui)
    :ui_element_t(ui) {
        reset();
        params = 0;
        last_used = 0;
    }

    ui_text_t(GLFWwindow *window, shaderProgram_t *shader, texture_t *texture, glm::vec4 xywh, std::string text = "", callback_t pre_render_callback = callback_t())
    :ui_element_t(window, xywh) {
        reset();
        params = 0;
        set_string(text);
        this->pre_render_callback = pre_render_callback;
        this->shader = shader;
        this->texture = texture;
    }

    virtual text_parameters *get_parameters() {
        return params ? params : &global_text_parameters;
    }

    virtual bool parameters_changed() {
        //return (last_used != params && last_used != &global_text_parameters) || last_used == nullptr;
        return false;
    }

    bool render() override {
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

    std::string get_element_name() override {
        const std::string basic_name = "ui_text_t";

        if (string_buffer.size() < 1)
            return basic_name;
        
        return basic_name + " (" + string_buffer + ")";
    }

    protected:
    bool mesh() override {
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
};