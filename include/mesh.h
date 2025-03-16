#pragma once

#include "common.h"
#include "texture.h"

struct vertex_t {
    glm::vec3 vertex, normal;
    glm::vec2 tex;
    glm::vec3 color;
};

struct mesh_t {
    std::vector<vertex_t> verticies;
    GLuint vao, vbo, vertexCount;
    glm::vec3 minBound, maxBound;
    glm::vec3 position;
    bool modified;
    
    mesh_t();

    ~mesh_t();

    bool loadStl(const char *filename);

    /*
    Blender export Y forward, Z up. Triangulation, UV, normals
    */
    bool loadObj(const char *filepath);

    virtual void clear();

    virtual void mesh();

    virtual void render();
};
