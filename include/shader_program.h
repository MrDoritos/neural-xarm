#pragma once

#include <glm/gtc/type_ptr.hpp>

#include "common.h"
#include "shader.h"
#include "camera.h"
#include "texture.h"

struct material_t {
    inline material_t(texture_t *diffuse, texture_t *specular, float gloss)
    :diffuse(diffuse),specular(specular),shininess(gloss) { }
    texture_t *diffuse, *specular;
    float shininess;

    inline void use(const int &unit = 0) const {
        diffuse->use(unit);
        specular->use(unit + 1);
    }
};

struct light_t {
    using v3 = glm::vec3;
    inline light_t(v3 position, v3 ambient, v3 diffuse, v3 specular)
    :position(position),ambient(ambient),diffuse(diffuse),specular(specular) { } 
    v3 position, ambient, diffuse, specular;
};

struct shader_program_t {
    GLuint programId;
    std::vector<shader_t*> shaders;
    std::unordered_map<std::string, GLint> resolved_locations;
    bool save_locations;
    static GLuint lastProgramId;

    inline shader_program_t():programId(gluninitialized),save_locations(true) { }

    template<typename ...Ts>
    inline shader_program_t(Ts ...shaders_)
    :shader_program_t(){
        this->shaders = {shaders_...};
     }

    inline shader_program_t(const shader_program_t &prg)
    :shader_program_t() {
        this->shaders = prg.shaders;
     }

    virtual bool get_uniform_locations() { return glsuccess; }

    virtual GLint get_uniform_location(const char *name) {
        const std::string _conv(name);

        if (resolved_locations.size() > 0) {
            if (resolved_locations.contains(_conv))
                return resolved_locations[_conv];
        }

        GLint loc = glGetUniformLocation(programId, name);

        if (save_locations && loc > -1) {
            resolved_locations[_conv] = loc;
        }

        assert(resolved_locations.size() < 100 && "resolved_locations is large");

        return loc;
    }

    template<typename ...Ts>
    bool resolve_uniform_locations(Ts ...names) {
        for (auto loc : std::initializer_list({names...})) {
            std::string _conv(loc);
            GLint resolved = get_uniform_location(_conv.c_str());
            resolved_locations[_conv] = resolved;
        }

        return glsuccess;
    }

    virtual void set_m4(const char *name, glm::mat4 mat) {
        glUniformMatrix4fv(get_uniform_location(name), 1, GL_FALSE, glm::value_ptr(mat));
    }

    virtual void set_m3(const char *name, glm::mat3 mat) {
        glUniformMatrix3fv(get_uniform_location(name), 1, GL_FALSE, glm::value_ptr(mat));
    }

    virtual void set_v3(const char *name, glm::vec3 vec) {
        glUniform3fv(get_uniform_location(name), 1, glm::value_ptr(vec));
    }

    virtual void set_v4(const char *name, glm::vec4 vec) {
        glUniform4fv(get_uniform_location(name), 1, glm::value_ptr(vec));
    }

    virtual void set_f(const char *name, float v) {
        glUniform1f(get_uniform_location(name), v);
    }

    virtual void set_i(const char *name, int v) {
        glUniform1i(get_uniform_location(name), v);
    }

    virtual glm::mat3 get_norm(glm::mat4 model) {
        return glm::transpose(glm::inverse(glm::mat3(model)));
    }

    virtual void set_camera(camera_t *camera, glm::mat4 model = glm::mat4(1.0f), std::string view_loc = "view", std::string model_loc = "model", std::string projection_loc = "projection", std::string norm_loc = "norm") {        
        auto view = camera->get_view_matrix();
        auto projection = camera->get_projection_matrix();

        set_m3(norm_loc.c_str(), get_norm(model));
        set_m4(view_loc.c_str(), view);
        set_m4(model_loc.c_str(), model);
        set_m4(projection_loc.c_str(), projection);
    }

    virtual void set_sampler(const char *name, texture_t *texture, int unit = 0) {
        assert(texture && "Texture null\n");
        texture->use(unit);
        set_i(name, unit);
    }

    virtual void use() {
        //if (lastProgramId != programId) {
            glUseProgram(programId);
        //    lastProgramId = programId;
        //}
    }

    bool load();
};