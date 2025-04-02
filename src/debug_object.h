#pragma once

#include "xarm_common.h"
#include "primitives.h"

namespace robot {

struct debug_object_t : public mesh_t {
    std::vector<std::pair<glm::vec3, glm::vec3>> _lines;
    std::vector<std::pair<glm::vec3, float>> _spheres;
    std::vector<std::pair<glm::mat4, float>> _circles;
    std::vector<std::pair<glm::vec3, glm::vec3>> _arrows;

    camera_t *camera;
    texture_t *circleTexture;
    shader_program_t *program;

    debug_object_t(camera_t *camera, shader_program_t *program, texture_t *circleTexture):
            camera(camera),
            circleTexture(circleTexture),
            program(program) {

    }

    void add_line(glm::vec3 origin, glm::vec3 end) {
        _lines.push_back({origin, end});
    }

    void add_sphere(glm::vec3 origin, float radius) {
        _spheres.push_back({origin, radius});
    }

    void add_circle(glm::mat4 matrix, float radius) {
        _circles.push_back({matrix, radius});
    }

    void add_arrow(glm::vec3 origin, glm::vec3 end) {
        _arrows.push_back({origin, end});
    }

    void clear() override {
        _lines.clear();
        _spheres.clear();
        _circles.clear();
        _arrows.clear();

        mesh_t::clear();
    }

    void add_rect(vertex_t *verts, unsigned int &vertexCount, const glm::mat4 &matrix, const float &radius, const glm::vec4 &UVWH, const glm::vec3 &color = {0.0f,0.0f,0.0f}) {
        const glm::vec4 fw = { radius,  radius, radius, 1.0f};
        const glm::vec4 bw = {-radius, -radius, -radius, 1.0f};
        const glm::vec3 origin = glm::vec3(matrix[3]);
        const float fac = sqrt(2.0f);
        const glm::vec3 a = matrix[2] * fw * fac;
        const glm::vec3 d = matrix[2] * bw * fac;
        const glm::vec3 b = matrix[0] * fw * fac;
        const glm::vec3 c = matrix[0] * bw * fac;

        const float &u = UVWH.x, &v = UVWH.y, &uw = UVWH.z, &vh = UVWH.w;

        struct pos {
            const glm::vec3 coords; const glm::vec2 tex; const glm::vec3 color;
        };

        const pos verticies[6] = {
            {a, {u,v}, color},
            {b, {u+uw,v}, color},
            {c, {u,v+vh}, color},
            {b, {u+uw,v}, color},
            {d, {u+uw,v+vh}, color},
            {c, {u,v+vh}, color}
        };

        for (int i = 0; i < 6; i++) {
            verts[vertexCount].vertex = verticies[i].coords + origin;
            verts[vertexCount].normal = matrix[1];
            verts[vertexCount].tex = verticies[i].tex;
            verts[vertexCount].color = verticies[i].color;

            vertexCount += 1;
        }
    }



    void render() override {
        program->use();
        program->set_camera(camera, glm::mat4(1.0f));

        float l = 0;
        for (auto &c : _circles) {
            vertex_t v[6];
            unsigned int g = 0;

            const glm::vec4 UVWH = {0,0,1,1};
            auto m = c.first;
            m = glm::translate(m, glm::vec3(0,l+=0.02,0));
            add_rect(&v[0], g, m, c.second, UVWH);

            std::copy(v, v+6, std::back_inserter(verticies));
            vertexCount += 6;
        }

        for (auto &s : _spheres) {
            vertex_t v[6];
            unsigned int g = 0;

            const glm::vec4 UVWH = {0,0,1,1};
            const glm::vec3 COLOR = {0,0,1};

            glm::vec3 ws = s.first;
            glm::mat4 matrix(1.0);
            matrix = glm::translate(matrix, ws);
            //matrix *= -camera->get_projection_matrix();
            glm::mat4 rmatrix(1.0);
            rmatrix = glm::rotate(rmatrix, -glm::radians(camera->yaw+90), glm::vec3(0,1,0));
            rmatrix = glm::rotate(rmatrix, glm::radians(camera->pitch+90), glm::vec3(1,0,0));
            matrix *= rmatrix;
            //auto to_view = glm::normalize(camera->position - ws);
            //matrix *= glm::lookAt(glm::vec3{0.0f}, glm::cross(camera->front, -camera->right), -camera->right);
            //matrix *= glm::lookAt(glm::vec3{0.0f}, -to_view, camera->up);

            add_rect(&v[0], g, matrix, s.second * 5, UVWH, COLOR);

            std::copy(v, v+6, std::back_inserter(verticies));
            vertexCount += 6;
        }

        glDisable(GL_DEPTH_TEST);
        program->set_sampler("material.diffuse", circleTexture, 0);
        program->set_sampler("material.specular", circleTexture, 1);
        modified = true;
        mesh_t::render();
        mesh_t::clear();

        for (auto &a : _arrows) {
            vertex_t v[6];
            unsigned int g = 0;
            
            const glm::vec4 UVWH = {0,0,1,1};
            const glm::vec3 COLOR = {1,0,0};

            auto &v1 = a.first;
            auto &v2 = a.second;
            glm::vec3 diff = a.first - a.second;
            glm::vec4 mag = glm::vec4(glm::normalize(diff), 0);
            glm::vec3 ws = a.first - (diff * 0.5f);
            glm::mat4 matrix(1.0);
            glm::mat4 m_billboard = camera->get_billboard_matrix();

            matrix = glm::translate(matrix, a.first);
            matrix = glm::rotate(matrix, (float)atan2(v1.x, v2.x), {0.0,0.0,1.0});
            matrix = glm::rotate(matrix, (float)atan2(v1.y, v2.y), {1.0,0.0,0.0});
            matrix = glm::scale(matrix, glm::vec3(1,1,glm::distance(a.first, a.second)));
            //matrix[0] = mag;
            //matrix[1] = mag;
            //matrix[2] = mag;

            matrix *= m_billboard;
            //std::swap(matrix[1], matrix[2]);
            //matrix *= camera->get_billboard_matrix();

            //glm::vec3 counter = glm::normalize(a.first - a.second);
            //glm::vec3 counter(1.0,5.0,1.0);
            //glm::vec3 counter = a.first - a.second;
            glm::vec3 counter(1.0);
            //matrix[1] = glm::vec4(glm::normalize(a.first - a.second), 1.0);

            //add_rect(&v[0], g, matrix, glm::distance(a.first, a.second), UVWH, COLOR);
            gui::add_rectangle_3d(&v[0], matrix, counter, UVWH, COLOR);

            std::copy(v, v+6, std::back_inserter(verticies));
            vertexCount += 6;
        }

        glDisable(GL_CULL_FACE);
        program->set_sampler("material.diffuse", arrowTexture, 0);
        program->set_sampler("material.specular", arrowTexture, 1);
        modified = true;
        mesh_t::render();

        program->set_sampler("material.diffuse", mainTexture, 0);
        program->set_sampler("material.specular", mainTexture, 1);
        mainTexture->use();

        const auto glv = [](const glm::vec3 &p) {
            glVertex3f(p.x, p.y, p.z);
        };

        const auto glvc = [glv](const glm::vec3 &v, const glm::vec3 &c) {
            glv(v);
            glColor4f(c.r, c.g, c.b, 1);
        };
        
        glm::vec3 colors[3] = {{1,0,0.},{0,1,0},{0,0,1}};
        int ic = 0;

        glBegin(GL_LINES);
        for (auto &l : _lines) {
            auto c = colors[ic++%3];
            //glv(l.first);
            //glv(l.second);
            glvc(l.first, c);
            glvc(l.second, c);
        }
        glEnd();

        this->clear();
    }    
};

}