#pragma once

#include "common.h"
#include "util.h"

template<typename mesh_base>
struct segment_T {
    segment_T(segment_T *parent, glm::vec3 rotation_axis, glm::vec3 initial_direction, float length, int servo_num) {
        this->parent = parent;
        this->rotation_axis = rotation_axis;
        this->initial_direction = initial_direction;
        this->length = length;
        this->servo_num = servo_num;
        this->rotation = 0.0f;
        this->mesh = new mesh_base();
    }

    ~segment_T() {
        if (this->mesh) {
            ~this->mesh();
            delete this->mesh;
        }
        this->mesh = nullptr;
    }

    constexpr inline float get_clamped_rotation(const bool &allow_interpolate = false) const {
        return util::clamp(allow_interpolate ? get_rotation() : rotation, -180, 180);
    }

    inline float get_rotation(const bool &allow_interpolate = true) const;

    constexpr inline float get_length() const {
        return length * model_scale;
    }

    constexpr inline glm::mat4 get_rotation_matrix(const bool &allow_interpolate = true) const {
        if (!parent)
            return glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(x_axis));

        return glm::rotate(parent->get_rotation_matrix(allow_interpolate), glm::radians(get_rotation(allow_interpolate)), rotation_axis);
    }

    constexpr inline glm::mat4 get_model_transform(const bool &allow_interpolate = true) const {
        const glm::vec3 origin = get_origin();
        glm::mat4 matrix(1.);
        
        matrix = glm::translate(matrix, origin);
        matrix *= get_rotation_matrix(allow_interpolate);
        matrix = glm::scale(matrix, glm::vec3(model_scale));
        
        return matrix;        
    }

    constexpr inline glm::vec3 get_segment_vector(const bool &allow_interpolate = true) const {
        return util::matrix_to_vector(get_rotation_matrix(allow_interpolate)) * get_length();
    }

    constexpr inline glm::vec3 get_origin(const bool &allow_interpolate = true) const {
        if (!parent) {
            assert(mesh && "Mesh null\n");
            return mesh->position;
        }

        if (servo_num == 5) { //shift forward just for this servo
            glm::mat4 iden = parent->get_rotation_matrix(allow_interpolate);
            auto trn = glm::vec3(2.54f * -model_scale, 0., 0.);
            iden = glm::translate(iden, trn);
            iden = glm::rotate(iden, glm::radians(90.0f), {0,0,1});

            return parent->get_segment_vector(allow_interpolate) + 
                   parent->get_origin(allow_interpolate) + 
                   glm::vec3(iden[3]);
        }

        return parent->get_segment_vector(allow_interpolate) + parent->get_origin(allow_interpolate);
    }

    void renderVector(glm::vec3 origin, glm::vec3 end) {
        debug_objects->add_line(origin, end);
    }

    void renderMatrix(glm::vec3 origin, glm::mat4 mat) {
        glm::vec4 m = glm::vec4(origin.x, origin.y, origin.z, 0);

        renderVector(origin, m + mat[0]);
        renderVector(origin, m + mat[1]);
        renderVector(origin, m + mat[2]);
    }

    void renderDebug() {
        glm::vec3 origin = get_origin();
        debug_objects->add_sphere(origin, model_scale);
        debug_objects->add_sphere(origin + get_segment_vector(), model_scale);
        renderVector(origin, origin + get_segment_vector());

        auto rot_mat = get_rotation_matrix();
        auto tran_rot_mat = glm::translate(rot_mat, origin);

        renderMatrix(origin, tran_rot_mat);
    }

    void render() {
        mainProgram->use();
        mainProgram->set_camera(camera, glm::mat4(1.0f));
        if (debug_mode)
            renderDebug();
        mainProgram->set_camera(camera, get_model_transform());
        if (debug_pedantic)
            mainProgram->set_v3("light.ambient", debug_color);
        mesh->render();
    }

    virtual bool load(const char *path) {
        assert(mesh && "Mesh null");
        return mesh->load(path);
    }

    float model_scale = 0.1;
    glm::vec3 direction, rotation_axis, initial_direction;
    segment_t *parent;
    glm::vec3 debug_color;
    float length, rotation;
    int servo_num;
    mesh_t *mesh;
};

using segment_t = segment_T<>;