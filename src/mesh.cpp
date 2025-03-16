#include "stl_reader.h"
#include <tiny_obj_loader.h>
#include "mesh.h"

mesh_t::mesh_t():
        vertexCount(0),
        vao(0),
        vbo(0),
        modified(0),
        position(0.0f) {
    clear();

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
}

mesh_t::~mesh_t() {
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void mesh_t::clear() {
    verticies.clear();

    vertexCount = 0;
    modified = false;
}

void mesh_t::mesh() {
    size_t coordSize = sizeof verticies[0].vertex;
    size_t normSize = sizeof verticies[0].normal;
    size_t texSize = sizeof verticies[0].tex;
    size_t colorSize = sizeof verticies[0].color;

    size_t stride = sizeof verticies[0];

    assert(vertexCount == verticies.size() && "vertexCount != verticies.size()\n");

    if (vertexCount < 1) {
        modified = false;
        return;
    }

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * stride, verticies.data(), GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*) (0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*) (coordSize));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*) (coordSize + normSize));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*) (coordSize + normSize + texSize));

    glBindVertexArray(0);

    if (debug_pedantic) {
        printf("Uploaded %i verticies, stride: %li, size: %li, vbo: %i, addr: %p\n", vertexCount, stride, vertexCount * sizeof verticies[0], vbo, verticies.data());
        printf("Vertex <min,max> <%f,%f><%f,%f><%f,%f>\n", minBound.x, maxBound.x, minBound.y, maxBound.y, minBound.z, maxBound.z);
    }

    modified = false;
}

void mesh_t::render() {
    if (modified)
        mesh();

    if (vertexCount < 1)
        return;

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    glBindVertexArray(0);
}

bool mesh_t::loadStl(const char *filename) {
    std::vector<float> coords, normals;
    std::vector<unsigned int> tris, solids;
    clear();
    stl_reader::ReadStlFile(filename, coords, normals, tris, solids);
    
    if (tris.size() < 1) {
        return glfail;
    }

    auto minVec = glm::vec3(std::numeric_limits<float>().max());
    auto maxVec = glm::vec3(std::numeric_limits<float>().min());

    minBound = minVec;
    maxBound = maxVec;

    auto minNormal = minVec;
    auto maxNormal = maxVec;

    vertexCount = tris.size();
    int triCount = vertexCount / 3;

    verticies.resize(vertexCount);

    {
        for (int iT = 0; iT < triCount; iT++) {
            float *n = &normals[3 * iT];

            for (int iV = 0; iV < 3; iV++) {
                int indexT = 3 * iT + iV;
                int indexV = 3 * tris[indexT];

                indexV = (iT * 3) + iV;
                float *v = &coords[3 * tris[3 * iT + iV]];

                verticies[indexV].vertex = {v[0], v[1], v[2]};
                verticies[indexV].normal = {n[0], n[1], n[2]};
                float ux[] = {0,0,1};
                float uy[] = {0,1,0};
                verticies[indexV].tex = {ux[indexV % 3], uy[indexV % 3] };
            }
        }

        auto set_min = [](glm::vec3 &a, const glm::vec3 &b) {
            for (int i = 0; i < 3; i++)
                a[i] = std::min(a[i], b[i]);
        };

        auto set_max = [](glm::vec3 &a, const glm::vec3 &b) {
            for (int i = 0; i < 3; i++)
                a[i] = std::max(a[i], b[i]);
        };

        for (int i = 0; i < vertexCount; i++) {
            auto vertex = verticies[i].vertex;

            set_max(maxBound, verticies[i].vertex);
            set_min(minBound, verticies[i].vertex);
            set_max(maxNormal, verticies[i].normal);
            set_min(minNormal, verticies[i].normal);
        }

    }

    modified = true;

    return glsuccess;
}

bool mesh_t::loadObj(const char *filepath) {
    tinyobj::attrib_t inattrib;
    std::vector<tinyobj::shape_t> inshapes;
    std::vector<tinyobj::material_t> inmaterials;
    std::map<std::string, texture_t> textures;

    std::string basedir = filepath;
    if (basedir.find_last_of("/\\") != std::string::npos)
        basedir = basedir.substr(0, basedir.find_last_of("/\\"));
    else
        basedir = ".";
    basedir += "/";

    std::string warn, err;
    bool ret = tinyobj::LoadObj(&inattrib, &inshapes, &inmaterials, &warn, &err, filepath, basedir.c_str());

    if (!warn.empty())
        fprintf(stderr, "ObjLoad warn: %s\n", warn.c_str());
    if (!err.empty())
        fprintf(stderr, "ObjLoad error: %s\n", err.c_str());
    if (!ret)
        return glfail;

    if (debug_mode) {
        fprintf(stderr, \
"obj file: %s\n\
vertex count: %i\n\
normal count: %i\n\
texcoord count: %i\n\
material count: %i\n\
shape count: %i\n",
        filepath, 
        (int)inattrib.vertices.size(), 
        (int)inattrib.normals.size(), 
        (int)inattrib.texcoords.size(), 
        (int)inmaterials.size(), 
        (int)inshapes.size());
    }

    inmaterials.push_back(tinyobj::material_t());

    if (debug_mode)
        for (auto &mat : inmaterials)
            fprintf(stderr, "material.diffuse_texname = %s\n",
                mat.diffuse_texname.c_str());
    
    for (auto &mat : inmaterials) {
        auto &fn = mat.diffuse_texname;

        if (!fn.length())
            continue;

        if (textures.contains(fn))
            continue;

        auto &tx = textures[fn];

        if (!tx.load(fn))
            continue;

        fprintf(stderr, "Failed to load texture %s\n", fn.c_str());
    }

    auto &attrib = inattrib;
    auto &materials = inmaterials;

    for (size_t s = 0; s < inshapes.size(); s++) {
        auto &shape = inshapes[s];
        auto &mesh = shape.mesh;
        auto &verts = attrib.vertices;
        auto &norms = attrib.normals;
        auto &texs = attrib.texcoords;
        auto &index = mesh.indices;
        auto vertCount = index.size();
        auto triCount = vertCount / 3;

        verticies.reserve(verticies.size() + vertCount);

        for (size_t f = 0; f < triCount; f++) {
            int material_id = mesh.material_ids[f];
            auto i0 = index[3 * f + 0];
            auto i1 = index[3 * f + 1];
            auto i2 = index[3 * f + 2];
            glm::vec<3, decltype(i2)> is = {i0,i1,i2};

            if (material_id < 0 || material_id >= materials.size())
                material_id = materials.size() - 1;

            glm::vec3 color;
            for (int i = 0; i < 3; i++) {
                color[i] = materials[material_id].diffuse[i];
            }

            vertex_t vnt[3];

            for (int k = 0; k < 3; k++) {
                glm::ivec3 vi, ni, ti;

                vnt[k].color = color;
                for (int i = 0; i < 3; i++) {
                    vi[i] = is[i].vertex_index;
                    ni[i] = is[i].normal_index;
                    ti[i] = is[i].texcoord_index;                        

                    auto &vert = vnt[i];
                    vert.vertex[k] = verts[3 * vi[i] + k];
                    vert.normal[k] = norms[3 * ni[i] + k];
                    if (k < 2)
                        vert.tex[k] = texs[2 * ti[i] + k];

                }
            }

            for (int i = 0; i < 3; i++)
                verticies.push_back(vnt[i]);
        }
    }

    vertexCount = verticies.size();
    modified = true;

    if (debug_mode)
        fprintf(stderr, "Final verticies: %i\n", vertexCount);

    return glsuccess;
}