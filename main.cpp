#include <iostream>
#include <limits>
#include <chrono>
#include <functional>
#include <algorithm>
#include <map>
#include <string>
#include <format>

#include <signal.h>

#include <GL/gl.h>
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <GLFW/glfw3.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <hidapi/hidapi.h>

#define STB_IMAGE_IMPLEMENTATION
#include "thirdparty/stb_image.h"
#include "thirdparty/stl_reader.h"

struct camera_t;
struct mesh_t;
struct texture_t;
struct shader_t;
struct shaderProgram_t;
struct shader_text_t;
struct shader_materials_t;
struct segment_t;
struct material_t;
struct kinematics_t;
struct debug_object_t;
struct debug_info_t;
struct ui_element_t;
struct ui_text_t;
struct ui_slider_t;
struct ui_toggle_t;
struct joystick_t;
struct robot_interface_t;

glm::vec<4, int> initial_window;
glm::vec<4, int> current_window;
bool fullscreen;
bool debug_mode = false;
bool userinput_kinematic = false;
float mouseSensitivity = 0.05f;
float preciseSpeed = 0.1f;
float movementSpeed = 10.0f;
float rapidSpeed = 20.0f;
GLFWwindow *window;
GLint uni_projection, uni_model, uni_norm, uni_view;
const GLuint gluninitialized = -1, glfail = -1, glsuccess = GL_NO_ERROR, glcaught = 1;
texture_t *textTexture, *mainTexture;
shader_t *mainVertexShader, *mainFragmentShader;
shader_t *textVertexShader, *textFragmentShader;
shader_text_t *textProgram;
shader_materials_t *mainProgram;
material_t *robotMaterial;
camera_t *camera;
segment_t *sBase, *s6, *s5, *s4, *s3;
std::vector<segment_t*> segments;
debug_object_t *debug_objects;
ui_text_t *debugInfo;
ui_toggle_t *debugToggle;
ui_slider_t *slider6, *slider5, *slider4, *slider3, *slider_ambient, *slider_diffuse, *slider_specular, *slider_shininess;
std::vector<ui_slider_t*> slider_whatever;
ui_element_t *uiHandler, *ui_servo_sliders;
kinematics_t *kinematics;
glm::vec3 robot_target;
joystick_t *joysticks;
robot_interface_t *robot_interface;

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
void cursor_position_callback(GLFWwindow *window, double x, double y);
void handle_keyboard(GLFWwindow* window, float deltaTime);
void joystick_callback(int jid, int event);
void handle_signal(int sig);
void handle_error(const char *str, int errcode = -1);
void reset();
void destroy();
void hint_exit();
void safe_exit(int errcode = 0);

using hrc = std::chrono::high_resolution_clock;
using tp = std::chrono::time_point<hrc>;
using dur = std::chrono::duration<float>;
tp start, end;
dur duration;
int frameCount = 0;
float fps;
double lastTime, deltaTime;

glm::vec4 x_axis(1,0,0,0), y_axis(0,1,0,0), z_axis(0,0,1,0);

struct camera_t {
    camera_t(glm::vec3 position = {0,0,0.}, float pitch = 0., float yaw = 0.) {
        position = glm::vec3(0.0f, 0.0f, 0.0f);
        up = glm::vec3(0.0f, 1.0f, 0.0f);
        front = glm::vec3(0.0f, 0.0f, -1.0f);
        right = glm::vec3(1.0f, 0.0f, 0.0f);
        fov = 90.0f;
        near = 0.1f;
        far = 1000.0f;
        pitch = 0.0f;
        yaw = 0.0f;
        last_mouse[0] = 0.0f;
        last_mouse[1] = 0.0f;

        this->position = position;
        this->pitch = pitch;
        this->yaw = yaw;

        calculate_normals();
    }

    glm::vec3 position, up, front, right;
    float fov, near, far, pitch, yaw;
    double last_mouse[2];
    bool no_mouse = true;
    bool interactive = false;

    glm::mat4 get_view_matrix() {
        return glm::lookAt(position, position + front, up);
    }

    glm::mat4 get_projection_matrix() {
        return glm::perspective(glm::radians(camera->fov),
                                (float) current_window[2] / current_window[3], 
                                camera->near, camera->far);
    }

    void calculate_normals() {
        front = glm::normalize(glm::vec3(
            std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch)),
            std::sin(glm::radians(pitch)),
            std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch))
        ));

        right = glm::cross(front, up);
    }

    bool isInteractive() {
        return interactive;
    }

    void mousePress(GLFWwindow *window, int button, int action, int mods) {
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            interactive = !interactive;
        } else
        if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS && interactive) {
            interactive = false;
        }

        if (interactive) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            no_mouse = true;
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    void mouseMove(GLFWwindow *window, double x, double y) {
        if (!isInteractive())
            return;

        double dx = x - last_mouse[0];
        double dy = last_mouse[1] - y;

        last_mouse[0] = x;
        last_mouse[1] = y;

        if (no_mouse) {
            no_mouse = false;
            return;
        }

        yaw += mouseSensitivity * dx;
        pitch += mouseSensitivity * dy;

        pitch = std::max(std::min(pitch, 89.9f), -89.9f);

        calculate_normals();
        //printf("LookAt: yaw: %f pitch: %f\n", yaw, pitch);
    }

    void joystick_move(GLFWwindow *window, float pitch, float yaw) {
        this->yaw += yaw;
        this->pitch += pitch;
        this->pitch = std::max(std::min(this->pitch, 89.9f), -89.9f);

        calculate_normals();
    }

    void keyboard(GLFWwindow *window, float deltaTime) {
        if (!isInteractive())
            return;

        float movementFactor = movementSpeed;
        bool precise = (glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);
        bool rapid = (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS);

        if (rapid && precise) {
            
        } else
        if (rapid) {
            movementFactor = rapidSpeed;
        } else
        if (precise) {
            movementFactor = preciseSpeed;
        }

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            position += front * deltaTime * movementFactor;
        }

        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            position -= front * deltaTime * movementFactor;
        }

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            position -= right  * deltaTime * movementFactor;
        }

        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            position += right * deltaTime * movementFactor;
        }

        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            position += up * deltaTime * movementFactor;
        }

        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            position -= up * deltaTime * movementFactor;
        }

        //printf("Position: x: %f, y: %f, z: %f\n", position.x, position.y, position.z);
    }
};

struct mesh_t {
    struct vertex_t {
        glm::vec3 vertex;
        glm::vec3 normal;
        glm::vec2 tex;
    };

    std::vector<vertex_t> verticies;
    GLuint vao, vbo, vertexCount;
    glm::vec3 minBound, maxBound;
    glm::vec3 position;
    bool modified;
    
    mesh_t() {
        clear();
        position = glm::vec3(0.0f);

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
    }

    ~mesh_t() {
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
    }

    virtual void clear() {
        verticies.clear();

        vertexCount = 0;
        modified = false;
    }

    bool load(const char *filename) {
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

        verticies.reserve(vertexCount);

        {
            for (int iT = 0; iT < triCount; iT++) {
                float *n = &normals[3 * iT];

                for (int iV = 0; iV < 3; iV++) {
                    int indexT = 3 * iT + iV;
                    int indexV = 3 * tris[indexT];

                    indexV = (iT * 3) + iV;
                    //float *v = &s3_test.coords[indexV];
                    float *v = &coords[3 * tris[3 * iT + iV]];
                    //float *v = &positionCube3[(iT * 9) + (iV * 3)];

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

    virtual void mesh() {
        size_t coordSize = sizeof verticies[0].vertex;
        size_t normSize = sizeof verticies[0].normal;
        size_t texSize = sizeof verticies[0].tex;

        size_t stride = sizeof verticies[0];


        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertexCount * stride, verticies.data(), GL_STATIC_DRAW);
        
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*) (0));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*) (normSize));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*) (normSize + texSize));

        glBindVertexArray(0);

        if (debug_mode) {
            //printf("Uploaded %i verticies, stride: %li, size: %li, vbo: %i, addr: %p\n", vertexCount, stride, vertexCount * sizeof verticies[0], vbo, verticies.data());
            //printf("Vertex <min,max> <%f,%f><%f,%f><%f,%f>\n", minBound.x, maxBound.x, minBound.y, maxBound.y, minBound.z, maxBound.z);
            //printf("Normal <min,max> <%f,%f><%f,%f><%f,%f>\n", minNormal.x, maxNormal.x, minNormal.y, maxNormal.y, minNormal.z, maxNormal.z);
        }

        modified = false;
    }

    virtual void render() {
        if (modified)
            mesh();

        if (!vertexCount)
            return;

        //printf("Draw %i with %i verticies\n", vbo, vertexCount);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        glBindVertexArray(0);
    }
};

struct texture_t {
    GLuint textureId;
    std::string path;

    texture_t(std::string path) :path(path),textureId(gluninitialized) { }

    texture_t() :path(),textureId(gluninitialized) { }

    bool isLoaded() {
        return textureId != gluninitialized;
    }

    void use(int unit) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, textureId);
    }

    void use() {
        glBindTexture(GL_TEXTURE_2D, textureId);
    }

    bool generate(glm::vec<4, float> rgba) {
        int width = 4, height = 4, channels = 4;
        int pixels = width * height;
        int size = pixels * channels;
        unsigned char* image = new unsigned char[size];

        glm::vec<4, unsigned char> vu = rgba * glm::vec4(255.0f);

        if (!image) {
            printf("Error generating image <%i,%i,%i,(%i),(%i),<%i,%i,%i,%i>>\n", width, height, channels, pixels, size, vu.r,vu.b,vu.g,vu.a);
            return glfail;
        }

        for (int i = 0; i < pixels; i++) {
            image[i * channels + 0] = vu.r;
            image[i * channels + 1] = vu.g;
            image[i * channels + 2] = vu.b;
            image[i * channels + 3] = vu.a;
        }

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        return glsuccess;
    }

    bool load() {
        int width, height, channels;
        unsigned char* image = stbi_load(path.c_str(), &width, &height, &channels, 0);

        if (!image) {
            std::cout << "Error opening image file: " << path << std::endl;
            return glfail;
        }

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        stbi_image_free(image);

        return glsuccess;
    }
};

struct shader_t {
    GLuint shaderId;
    GLenum type;
    std::string path;

    shader_t(std::string path, GLenum type)
    :path(path),type(type),shaderId(gluninitialized) { }

    bool isLoaded() {
        return shaderId != gluninitialized;
    }

    bool load() {
        std::stringstream buffer;
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Error opening shader file: " << path << std::endl;
            return glfail;
        }
        buffer << file.rdbuf();
        std::string shaderCodeStr = buffer.str();
        const char* shaderCode = shaderCodeStr.c_str();
        int length = shaderCodeStr.size();
        shaderId = glCreateShader(type);
        glShaderSource(shaderId, 1, &shaderCode, 0);
        glCompileShader(shaderId);
        GLint compiled = 0;
        glGetShaderiv(shaderId, GL_COMPILE_STATUS, &compiled);
        GLint infoLen = 512;
        if (compiled == GL_FALSE) {
            char infoLog[infoLen];
            glGetShaderInfoLog(shaderId, infoLen, nullptr, infoLog);
            std::cerr << "Error compiling shader: " << path << std::endl;
            std::cerr << infoLog << std::endl;
            //std::cerr << shaderCodeStr << std::endl;
            glDeleteShader(shaderId);
            return glfail;
        }

        return glsuccess;
    }
};

struct shaderProgram_t {
    GLuint programId;
    std::vector<shader_t*> shaders;
    std::unordered_map<std::string, GLint> resolved_locations;
    bool save_locations;

    shaderProgram_t():programId(gluninitialized),save_locations(true) {

    }

    template<typename ...Ts>
    shaderProgram_t(Ts ...shaders_)
    :shaderProgram_t(){
        this->shaders = {shaders_...};
     }

    shaderProgram_t(const shaderProgram_t &prg)
    :shaderProgram_t() {
        this->shaders = prg.shaders;
     }

    virtual bool get_uniform_locations() { return glsuccess; }

    virtual GLint get_uniform_location(const char *name) {
        if (resolved_locations.size() > 0) {
            std::string _conv(name);
            if (resolved_locations.find(_conv) != resolved_locations.end())
                return resolved_locations[_conv];
        }

        GLint loc = glGetUniformLocation(programId, name);

        if (save_locations) {
            std::string _conv(name);
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
        glm::mat3 norm(model);
        norm = glm::inverse(norm);
        norm = glm::transpose(norm);
        //return norm;
        return glm::transpose(glm::inverse(glm::mat3(1.)));
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
        texture->use(unit);
        set_i(name, unit);
    }

    virtual void use() {
        glUseProgram(programId);
    }

    bool load() {
        for (auto *shader : shaders)
            if (!shader->isLoaded())
                if (shader->load() != glsuccess) {
                    std::cout << "Shader failed to load, will not link shader" << std::endl;
                    return glfail;
                } else {
                    if (shader->shaderId == gluninitialized) {
                        std::cout << "shaderId is uninitialized" << std::endl;
                        return glfail;
                    }
                }

        programId = glCreateProgram();
        
        for (auto *shader : shaders)
            glAttachShader(programId, shader->shaderId);

        glLinkProgram(programId);

        int success = 0;
        glGetProgramiv(programId, GL_LINK_STATUS, &success);
        if (success == GL_FALSE) {
            char infoLog[513];
            glGetProgramInfoLog(programId, 512, NULL, infoLog);
            std::cout << "Error linking shader\n" << infoLog << std::endl;
            return glfail;
        }

        return glsuccess && get_uniform_locations();
    }
};

struct material_t {
    material_t(texture_t *diffuse, texture_t *specular, float gloss)
    :diffuse(diffuse),specular(specular),shininess(gloss) { }
    texture_t *diffuse, *specular;
    float shininess;

    void use(int unit = 0) {
        diffuse->use(unit);
        specular->use(unit + 1);
    }
};

struct light_t {
    using v3 = glm::vec3;
    light_t(v3 position, v3 ambient, v3 diffuse, v3 specular)
    :position(position),ambient(ambient),diffuse(diffuse),specular(specular) { } 
    v3 position, ambient, diffuse, specular;
};

struct shader_materials_t : public shaderProgram_t {
    shader_materials_t(shaderProgram_t prg)
    :shaderProgram_t(prg),material(0),light(0) { }

    material_t *material;
    light_t *light;
    glm::vec3 eyePos;

    void set_material(material_t *mat) {
        this->material = mat;
        set_i("material.diffuse", mat->diffuse->textureId);
        set_i("material.specular", mat->specular->textureId);
        set_f("material.shininess", mat->shininess);
    }

    void set_light(light_t *light) {
        this->light = light;
        set_v3("light.position", light->position);
        set_v3("light.ambient", light->ambient);
        set_v3("light.diffuse", light->diffuse);
        set_v3("light.specular", light->specular);
    }

    void set_eye(glm::vec3 eyePos) {
        this->eyePos = eyePos;
        set_v3("eyePos", eyePos);
    }

    void use() override {
        shaderProgram_t::use();

        if (material)
            material->use();

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glDisable(GL_CULL_FACE);
        glEnable(GL_ALPHA_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
};

struct shader_text_t : public shaderProgram_t {
    shader_text_t(shaderProgram_t prg)
    :shaderProgram_t(prg),mixFactor(0.0) { }

    float mixFactor;

    void use() override {
        shaderProgram_t::use();

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glDisable(GL_CULL_FACE);        

        set_m4("projection", glm::mat4(1.));
        set_f("mixFactor", mixFactor);
    }
};

struct debug_object_t : public mesh_t {
    std::vector<std::pair<glm::vec3, glm::vec3>> _lines;
    std::vector<std::pair<glm::vec3, float>> _spheres;

    void add_line(glm::vec3 origin, glm::vec3 end) {
        _lines.push_back({origin, end});
    }

    void add_sphere(glm::vec3 origin, float radius) {
        _spheres.push_back({origin, radius});
    }

    void clear() override {
        _lines.clear();
        _spheres.clear();

        mesh_t::clear();
    }

    void render() override {
        mainProgram->use();
        mainProgram->set_camera(camera, glm::mat4(1.0f));

        modified = true;
        mesh_t::render();

        glBegin(GL_LINES);
        for (auto &l : _lines) {
            glVertex3f(l.first.x, l.first.y, l.first.z);
            glVertex3f(l.second.x, l.second.y, l.second.z);
        }
        glEnd();

        glBegin(GL_QUADS);
        for (auto &s : _spheres) {
            glm::vec3 ws = s.first;
            float r = s.second * 5;
            //ws = ws / (camera->front * glm::mat3(camera->get_projection_matrix()));
            glm::vec3 cr = camera->right * glm::vec3(0.5f) * r;
            glm::vec3 cu = camera->up * glm::vec3(0.5f) * r;
            glm::vec3 v1 = ws + cr + cu,
                      v2 = ws + cr - cu, 
                      v3 = ws - cr - cu, 
                      v4 = ws - cr + cu;

            auto glv = [](glm::vec3 p) {
                glVertex3f(p.x, p.y, p.z);
            };

            glv(v1);
            glv(v2);
            glv(v3);
            glv(v4);
            /*
            glVertex3f(ws.x - r, ws.y - r, ws.z);
            glVertex3f(ws.x - r, ws.y + r, ws.z);
            glVertex3f(ws.x + r, ws.y + r, ws.z);
            glVertex3f(ws.x + r, ws.y - r, ws.z);
            */
        }
        glEnd();

        this->clear();
    }    
};

struct segment_t : public mesh_t {
    segment_t(segment_t *parent, glm::vec3 rotation_axis, glm::vec3 initial_direction, float length, int servo_num) {
        this->parent = parent;
        this->rotation_axis = rotation_axis;
        this->initial_direction = initial_direction;
        this->length = length;
        this->servo_num = servo_num;
        this->rotation = 0.0f;
        //this->rotation = 12.0f;
    }

    static glm::vec3 matrix_to_vector(glm::mat4 mat) {
        return glm::normalize(glm::vec3(mat[2]));
        //return glm::normalize(glm::vec3(mat * glm::vec4(1.)));
        //return glm::normalize(glm::mat3(mat) * glm::vec3(1.));
    }

    static inline constexpr float clamp(const float &v, const float &min, const float &max) {
        float ret = v;
        const float r = max - min;
        while (ret < min) ret += r;
        while (ret > max) ret -= r;
        return ret;
    }

    inline float get_clamped_rotation() {
        return clamp(rotation, -180, 180);
    }

    float get_length() {
        return length * model_scale;
    }

    static glm::mat4 vector_to_matrix(glm::vec3 vec) {
        glm::vec4 hyp_vec = glm::vec4(vec.x, vec.y, vec.z, 1.0f);
        return glm::mat4(1.0f) * glm::mat4(hyp_vec,hyp_vec,hyp_vec,hyp_vec);
    }

    static glm::vec3 rotate_vector_3d(glm::vec3 vec, glm::vec3 axis, float degrees) {
        float radians = glm::radians(degrees);
        glm::mat4 rtn_matrix = vector_to_matrix(vec);

        return matrix_to_vector(glm::rotate(rtn_matrix, radians, axis));
    }

    glm::mat4 get_rotation_matrix() {
        if (!parent)
            return glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(x_axis));

        return glm::rotate(parent->get_rotation_matrix(), glm::radians(rotation), rotation_axis);
    }

    glm::mat4 get_model_transform() {
        //model_scale = 1.0;
        
        glm::vec3 origin = get_origin();
        glm::mat4 matrix(1.);
        
        matrix = glm::translate(matrix, origin);
        matrix *= get_rotation_matrix();
        matrix = glm::scale(matrix, glm::vec3(model_scale));
        
        return matrix;        
    }

    glm::vec3 get_segment_vector() {
        return matrix_to_vector(get_rotation_matrix()) * get_length();
    }

    glm::vec3 get_origin() {
        if (!parent)
            return position;

        if (servo_num == 5) { //shift forward just for this servo
            glm::mat4 iden = parent->get_rotation_matrix();
            auto trn = glm::vec3(2.54f * -model_scale, 0., 0.);
            iden = glm::translate(iden, trn);
            iden = glm::rotate(iden, glm::radians(90.0f), {0,0,1});

            return parent->get_segment_vector() + 
                   parent->get_origin() + 
                   glm::vec3(iden[3]);
        }

        return parent->get_segment_vector() + parent->get_origin();
    }

    void renderVector(glm::vec3 origin, glm::vec3 end) {
        /*
        glBegin(GL_LINES);
            glColor3f(1.0,0,0);
            glVertex3f(origin.x,origin.y,origin.z);
            glVertex3f(end.x,end.y,end.z);
        glEnd();
        */
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

        /*
        std::cout << "Servo: " << servo_num << std::endl;
        std::cout << "Origin vec3d\n" << glm::to_string(origin) << std::endl;
        std::cout << "Rotation matrix\n" << glm::to_string(rot_mat) << std::endl;
        std::cout << "Rotation matrix translated\n" << glm::to_string(tran_rot_mat) << std::endl;
        */

        renderMatrix(origin, tran_rot_mat);
    }

    void render() {
        mainProgram->use();
        mainProgram->set_camera(camera, glm::mat4(1.0f));
        if (debug_mode)
            renderDebug();
        mainProgram->set_camera(camera, get_model_transform());
        if (debug_mode)
            mainProgram->set_v3("light.ambient", debug_color);
        mesh_t::render();
    }

    float model_scale = 0.1;
    glm::vec3 direction, rotation_axis, initial_direction;
    segment_t *parent;
    glm::vec3 debug_color;
    float length, rotation;
    int servo_num;
};

struct text_parameters {
    using v4 = glm::vec4;

    text_parameters() {
        texCharacterCountX = 16; texCharacterCountY = 16;

        scrScale = 1.; scrScaleX = 1.; scrScaleY = 1.;
        scrCharacterSpacingScale = 2.; scrCharacterSpacingScaleX = .5; scrCharacterSpacingScaleY = .8;
        texCharacterTrim = 1.; texCharacterTrimX = 4.; texCharacterTrimY = 16.;
        scrCharacterTrim = 1.; scrCharacterTrimX = 0.; scrCharacterTrimY = 0.;

        scrCharacterTrimX = 0.0121;
        scrCharacterTrimY = 0.00570;
        texCharacterTrimX = 8.73;
        texCharacterTrimY = 15.7;
        scrCharacterSpacingScaleX = 0.212;
        scrCharacterSpacingScaleY = 0.424;
        scrScaleX = .7;//0.546;
        scrScaleY = .7;//0.633;
    }

    text_parameters(int count, float scale, float spacing, float padding, float margin)
    :text_parameters() {
        texCharacterCountX = count;
        texCharacterCountY = count;

        scrScale = scale;
        scrCharacterSpacingScale = spacing;
        texCharacterTrim = padding;
        scrCharacterTrim = margin;
    }

    friend bool operator==(const text_parameters & lhs, const text_parameters & rhs) {
        return std::addressof(lhs) == std::addressof(rhs);
    }

    friend bool operator!=(const text_parameters & lhs, const text_parameters & rhs) {
        return !(lhs == rhs);
    }

    int texCharacterCountX, texCharacterCountY;
    float scrScale, scrScaleX, scrScaleY,
          scrCharacterSpacingScale, scrCharacterSpacingScaleX, scrCharacterSpacingScaleY,
          texCharacterTrim, texCharacterTrimX, texCharacterTrimY,
          scrCharacterTrim, scrCharacterTrimX, scrCharacterTrimY;

    void calculate(int character, int currentX, int currentY, v4 SCR_XYWH, v4 &CHAR_XYWH, v4 &UV_XYWH) {
        //number of characters in x/y of texture
        //const float texCharacterCountX = 16., texCharacterCountY = 16.;

        //final scale coefficient
        //const float scrScale = 1.0f;
        const float _scrScaleX = scrScaleX * scrScale;
        const float _scrScaleY = scrScaleY * scrScale;

        //dimensions in uv space
        const float _texCharacterWidth = 1.0f / texCharacterCountX;
        const float _texCharacterHeight = 1.0f / texCharacterCountY;

        //spacing coefficient
        //const float scrCharacterSpacingScale = 2.;
        const float _scrCharacterSpacingScaleX = scrCharacterSpacingScaleX * scrCharacterSpacingScale;
        const float _scrCharacterSpacingScaleY = scrCharacterSpacingScaleY * scrCharacterSpacingScale;

        //initial dimensions in screen space
        const float _scrCharacterSpacingX = _texCharacterWidth * _scrScaleX;
        const float _scrCharacterSpacingY = _texCharacterHeight * _scrScaleY;

        //initial position given by element
        const float _elementOriginX = SCR_XYWH.x;
        const float _elementOriginY = SCR_XYWH.y;

        //current character xy to index uv
        const int _characterIndexX = (character % texCharacterCountX);
        const int _characterIndexY = (character / texCharacterCountX % texCharacterCountY);

        //trim uv
        //const float texCharacterTrim = 1.;
        const float _texCharacterTrimX = _texCharacterWidth/texCharacterTrimX/texCharacterTrim;//texCharacterWidth / 5.; //pad uv x
        const float _texCharacterTrimY = _texCharacterHeight/texCharacterTrimY/texCharacterTrim;//texCharacterHeight / 16.;

        //trim scr
        //const float scrCharacterTrim = 1.;
        const float _scrCharacterTrimX = scrCharacterTrimX * scrCharacterTrim;//scrCharacterSpacingX / 10.;
        const float _scrCharacterTrimY = scrCharacterTrimY * scrCharacterTrim;//scrCharacterSpacingY / 10.;

        //uv origin x/y
        const float _texOriginX = _characterIndexX * _texCharacterWidth + _texCharacterTrimX;
        const float _texOriginY = _characterIndexY * _texCharacterHeight + _texCharacterTrimY;

        //final uv w/h
        const float _texWidth = _texCharacterWidth - _texCharacterTrimX;
        const float _texHeight = _texCharacterHeight - _texCharacterTrimY;

        //screen origin x/y
        const float _scrOriginX = (currentX * _scrCharacterSpacingX * _scrCharacterSpacingScaleX) + _elementOriginX + _scrCharacterTrimX;
        const float _scrOriginY = (currentY * _scrCharacterSpacingY * _scrCharacterSpacingScaleY) + _elementOriginY + _scrCharacterTrimY;

        //final screen w/h
        const float _scrWidth = _scrCharacterSpacingX - _scrCharacterTrimX;
        const float _scrHeight = _scrCharacterSpacingY - _scrCharacterTrimY;

        //if (character == 'F')
        //    printf("F<%f,%f,%f,%f><%f,%f,%f,%f>\n", _scrOriginX, _scrOriginY, _scrWidth, _scrHeight, _texOriginX, _texOriginY, _texWidth, _texHeight);

        CHAR_XYWH = {_scrOriginX, _scrOriginY, _scrWidth, _scrHeight};
        UV_XYWH = {_texOriginX, _texOriginY, _texWidth, _texHeight};
    }
};

text_parameters global_text_parameters;

struct text_data {
    using vec6 = glm::vec<6, float>;
    union {
        struct {float x, y, u, v, b, a;};
        float data[6];
    };

    glm::vec2 &coords() { return *reinterpret_cast<glm::vec2*>(&data[0]); }
    glm::vec2 *coords_ptr() { return reinterpret_cast<glm::vec2*>(&data[0]); }
    glm::vec4 &tex_coords() { return *reinterpret_cast<glm::vec4*>(&data[2]); }
    glm::vec4 *tex_coords_ptr() { return reinterpret_cast<glm::vec4*>(&data[3]); }
    vec6 *all_coords_ptr() { return reinterpret_cast<vec6*>(&data[0]); }
    vec6 &all_coords() { return *reinterpret_cast<vec6*>(&data[0]); }
};

struct text_t : public text_data {
    template<typename ...Ts>
    void set_values(Ts... values) {
        int i = 0;
        auto i_list = std::initializer_list<float>({values...});
        for (auto v : i_list)
            data[i++] = v;
    }

    text_t(glm::vec2 coords, glm::vec4 texCoords) {
        this->coords() = coords;
        this->tex_coords() = texCoords;
    }

    text_t(float x, float y, float u, float v, float b = 0., float a = 0.)
    :text_t({x,y},{u,v,b,a}) { }

    text_t() { }

    void set_attrib_pointers() {
        //printf("size %i\n", sizeof *this);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof * this, (const void*) (0));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof * this, (const void*) (8));
        glEnableVertexAttribArray(1);
    }
};

void add_rect(text_t *buffer, unsigned int &vertexCount, glm::vec4 XYWH, glm::vec4 UVWH = glm::vec4(0,0,1,1.), bool is_color = false) {
    float x = XYWH[0], y = XYWH[1], w = XYWH[2], h = XYWH[3];
    float u = UVWH[0], v = UVWH[1], uw = UVWH[2], vh = UVWH[3];

    text_t verticies[6] = {
        {x,y,u,v},
        {x+w,y,u+uw,v},
        {x,y+h,u,v+vh},
        {x+w,y,u+uw,v},
        {x+w,y+h,u+uw,v+vh},
        {x,y+h,u,v+vh}
    };

    for (int i = 0; i < 6; i++) {
        buffer[vertexCount] = verticies[i];

        if (is_color)
            buffer[vertexCount].tex_coords() = UVWH;

        vertexCount++;
    }
}

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

        auto ret = run_children(&ui_element_t::render);

        if (pre_render_callback)
            pre_render_callback();

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
        return true;
    }

    virtual bool set_visible() {
        this->hidden = false;
        auto ret = run_children(&ui_element_t::set_visible);
        return true;
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
        //printf("cursor <%f,%f> win <%f,%f,%f,%f>\n", cursor_pos.x, cursor_pos.y, XYWH[0], XYWH[1], XYWH[2], XYWH[3]);
        return (cursor_pos.x >= XYWH.x && cursor_pos.y >= XYWH.y &&
                cursor_pos.x <= XYWH.x + XYWH[2] && cursor_pos.y <= XYWH.y + XYWH[3]);
    }

    virtual std::string get_element_name() {
        return "ui_element_t";
    }

    virtual bool load() {
        run_children(&ui_element_t::load);

        if (loaded) {
            if (debug_mode)
                puts("Attempt to load twice");
            return glfail;
        }

        glGenBuffers(1, &vbo);
        glGenVertexArrays(1, &vao);
        vertexCount = 0;
        modified = true;
        loaded = true;

        if (debug_mode)
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
    //using void(ui_element_t::_func*, Ts...);

    template<typename Tfunc, typename... Ts>
    bool run_children(Tfunc func, Ts... values) {
        for (auto *element : children)
            //((_func<Ts...>*)func)(element, values...);
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

struct ui_text_t : public ui_element_t {
    std::string string_buffer;
    int currentX, currentY;

    text_parameters *params, *last_used;

    void string_change() {
        modified = true;
    }

    void set_string(std::string str) {
        string_buffer = str;
        string_change();
    }

    void set_string(const char* str) {
        set_string(std::string(str));
    }

    void add_line(std::string str) {
        string_buffer += str + '\n';
        string_change();
    }

    void add_string(std::string str) {
        string_buffer += str;
        string_change();
    }

    bool reset() override {
        ui_element_t::reset();

        //puts("text reset");

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

    ui_text_t(GLFWwindow *window, glm::vec4 xywh, std::string text = "", callback_t pre_render_callback = callback_t())
    :ui_element_t(window, xywh) {
        reset();
        params = 0;
        set_string(text);
        this->pre_render_callback = pre_render_callback;
    }

    virtual text_parameters *get_parameters() {
        return params ? params : &global_text_parameters;
    }

    virtual bool parameters_changed() {
        return (last_used != params && last_used != &global_text_parameters) || last_used == nullptr;
    }

    bool render() override {
        if (hidden)
            return glsuccess;

        ui_element_t::render();

        if (parameters_changed()) {
            //puts("Parameters changed");
            modified = true;
        }

        if (modified)
            mesh();

        textProgram->use();
        textProgram->set_sampler("textureSampler", textTexture);
        //printf("%s %i verticies\n", get_element_name().c_str(), vertexCount);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        glBindVertexArray(0);

        return glsuccess;
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

        //printf("Text buffer: %i verticies\n", vertexCount);

        delete [] buffer;

        return glsuccess;
    }
};

float title_height = 0.0483;
float value_subpos_x = 0.06167;
float value_subpos_y = 0.0117;
float slider_frac_w = 1.;
float slider_frac_h = 0.5;
float granularity = 0.005;
float precise_factor = 20.;

struct ui_slider_t : public ui_element_t {
    using ui_slider_v = float;
    using callback_t = std::function<void(ui_slider_t*, ui_slider_v)>;

    ui_slider_v min, max, value, drag_value, initial_value;
    bool cursor_drag, limit, skip_text;
    glm::vec2 cursor_start, drag_start;
    callback_t value_change_callback;

    ui_text_t *title_text, *min_text, *max_text, *value_text;
    float title_pos_y;
    float text_pos_y;
    std::string title_cached;

    ui_slider_t(GLFWwindow *window, glm::vec4 XYWH, ui_slider_v min, ui_slider_v max, ui_slider_v value, std::string title = std::string(), bool limit = true, callback_t value_change_callback = callback_t(), bool skip_text = false, bool hidden = false)
    :ui_element_t(window, XYWH),min(min),max(max),value(value),initial_value(value),cursor_drag(false),limit(limit),skip_text(skip_text),title_cached(title) {
        title_text = min_text = max_text = value_text = 0;
        title_pos_y = text_pos_y = 0;
        this->hidden = hidden;
        if (!skip_text) {
            if (title.size() > 0) {
                title_text = add_child(new ui_text_t(window, XYWH));
            }
            min_text = add_child(new ui_text_t(window, XYWH));
            max_text = add_child(new ui_text_t(window, XYWH));
            value_text = add_child(new ui_text_t(window, XYWH));
        }

        arrange_text();

        this->value_change_callback = value_change_callback;
    }

    virtual void set_min(ui_slider_v min) {
        this->min = min;
        if (min_text) {
            min_text->XYWH = XYWH + glm::vec4(-value_subpos_x,text_pos_y,0,0);
            min_text->set_string(vtos(min));
        }
        modified = true;
    }

    virtual void set_max(ui_slider_v max) {
        this->max = max;
        if (max_text) {
            max_text->XYWH = XYWH + glm::vec4(XYWH[2]-value_subpos_x,text_pos_y,0,0);
            max_text->set_string(vtos(max));
        }
        modified = true;
    }

    virtual std::string vtos(ui_slider_v v) {
        const int buflen = 100;
        char buf[buflen];
        snprintf(buf, buflen, "%.1f", v);
        return std::string(buf);
    }

    virtual void set_value(ui_slider_v v) {
        ui_slider_v _vset = v;
        if (limit)
            _vset = clip(_vset, min, max);
        //printf("value <%f,%f>\n", v, _vset);
        if (value_text) {
            auto slider = get_slider_position();
            value_text->XYWH = glm::vec4(slider.x, text_pos_y + XYWH.y - value_subpos_y, 0,0);
            value_text->set_string(vtos(value));
        }
        this->value = _vset;
        if (value_change_callback)
            value_change_callback(this, this->value);
        modified = true;
    }

    virtual void set_title(std::string title = std::string()) {
        if (title_text) {
            title_pos_y = -title_height;
            title_text->XYWH = XYWH + glm::vec4(0,title_pos_y,0,title_height);
            if (title.size() > 0)
                title_cached = title;
            title_text->set_string(title_cached);
        }
    }

    virtual bool arrange_text() {
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

    bool reset() override {
        //Resets children text
        ui_element_t::reset();

        if (skip_text)
            return glsuccess;

        //Sets the text after a reset
        value = initial_value;
        arrange_text();

        return glsuccess;
    }

    std::string get_element_name() override {
        return "ui_slider_t";
    }

    ui_slider_v clip(ui_slider_v val, ui_slider_v b1, ui_slider_v b2) {
        ui_slider_v min = std::min(b1, b2);
        ui_slider_v max = std::max(b1, b2);

        if (val < min)
            return min;
        if (val > max)
            return max;
        return val;

        //return std::min(std::max(value, min), max);
    }

    bool onCursor(double x, double y) override {
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

    glm::vec4 get_slider_position() {
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

    bool onMouse(int button, int action, int mods) override {
        if (hidden)
            return glsuccess;

        if (cursor_drag && button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
            cursor_drag = false;
            userinput_kinematic = true;
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

    bool mesh() override {
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

    bool render() override {
        if (modified)
            mesh();

        if (vertexCount < 1 || hidden)
            return glsuccess;

        textProgram->use();
        textProgram->set_f("mixFactor", 1.0);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        glBindVertexArray(0);

        ui_element_t::render();
        
        return glsuccess;
    }
};

struct ui_toggle_t : public ui_element_t {
    using toggle_callback_t = std::function<void(ui_toggle_t*,bool)>;
    toggle_callback_t toggle_callback;

    bool toggle_state, held, initial_state;
    std::string cached_title;
    ui_text_t *title_text;

    ui_toggle_t(GLFWwindow *window, glm::vec4 XYWH, std::string title = std::string(), bool initial_state = false, toggle_callback_t toggle_callback = toggle_callback_t())
    :ui_element_t(window, XYWH),toggle_state(initial_state),initial_state(initial_state),held(false),cached_title(title),toggle_callback(toggle_callback) {
        if (cached_title.size() > 0)
            title_text = add_child(new ui_text_t(window, XYWH - glm::vec4(title_height/4.,title_height,0,0), cached_title));
    }

    bool onMouse(int key, int action, int mods) override {
        if (is_cursor_bound() && key == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_RELEASE) {
                set_toggle();
                set_held(false);
            } else
            if (action == GLFW_PRESS) {
                set_held(true);
            }
            return glcaught;
        }

        return glsuccess;
    }

    void set_toggle() {
        set_state(!toggle_state);
    }

    void set_held(bool state) {
        if (held != state) {
            modified = true;
            held = state;
        }
    }

    void set_state(bool state) {
        modified = true;
        toggle_state = state;
        if (toggle_callback)
            toggle_callback(this, toggle_state);
    }

    bool reset() override {
        ui_element_t::reset();

        set_held(false);
        set_state(initial_state);
        if (title_text)
            title_text->set_string(cached_title);

        return glsuccess;
    }

    bool mesh() override {
        modified = false;
        vertexCount = 0;

        text_t *buffer = new text_t[2 * 6];

        glm::vec4 button_held_color(0,0,0.5,1.);
        glm::vec4 button_on_color(0,0,1.0,1.);
        glm::vec4 button_off_color(0,0,0.1,1.);

        glm::vec4 color = (held ? button_held_color : (toggle_state ? button_on_color : button_off_color));

        add_rect(buffer, vertexCount, XYWH, color, true);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof * buffer, buffer, GL_STATIC_DRAW);

        buffer->set_attrib_pointers();

        delete [] buffer;

        return glsuccess;
    }

    bool render() override {
        if (modified)
            mesh();

        if (vertexCount < 1)
            return glsuccess;

        textProgram->use();
        textProgram->set_f("mixFactor", 1.0);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        glBindVertexArray(0);

        ui_element_t::render();

        return glsuccess;
    }
};

struct kinematics_t {
    static glm::vec3 map_to_xy(glm::vec3 pos, float r = 0.0f, glm::vec3 a = -z_axis, glm::vec3 o = glm::vec3(0.0f), glm::vec3 t = glm::vec3(0.0f)) {
        //auto mapped = segment_t::rotate_vector_3d(pos - o, a, r);
        //auto matrix = glm::translate(glm::mat4(1.0f), pos - o);
        auto matrix = glm::mat4(1.0);
        matrix = glm::rotate(matrix, glm::radians(r), a);
        //auto npos = glm::vec3(matrix[3].x, matrix[3].z, 0);
        //auto _4npos = glm::vec4(matrix[0] + matrix[1] + matrix[2]);
        //auto npos = glm::vec3(_4npos[0], _4npos[2], _4npos[1]);
        //return npos + t;
        auto _pos = pos - o;
        //_pos = pos;
        //matrix = glm::translate(matrix, -o);

        return glm::vec3(matrix * glm::vec4(_pos.x,_pos.y,_pos.z,1)) + t;
    }

    bool solve_inverse(glm::vec3 coordsIn) {
        auto isnot_real = [](float x){
            return (isinf(x) || isnan(x));
        };

        bool calculation_failure = false;

        auto target_coords = coordsIn;
        target_coords.z = -target_coords.z;
        auto target_2d = glm::vec2(target_coords.x, target_coords.z);

        float rot_out[5];

        for (int i = 0; i < segments.size(); i++)
            rot_out[i] = segments[i]->rotation;

        // solve base rotation
        auto seg_6 = s6->get_origin();
        auto seg2d_6 = glm::vec2(seg_6.x, seg_6.z);

        auto dif_6 = target_2d - seg2d_6;
        auto atan2_6 = atan2(dif_6[1], dif_6[0]);
        auto norm_6 = atan2_6 / M_PI;
        auto rot_6 = (norm_6 + 1.0f) / 2.0f;
        auto deg_6 = rot_6 * 360.0f;
        auto serv_6 = rot_6;

        //s6->rotation = serv_6;

        glm::vec3 seg_5 = s5->get_origin();
        glm::vec3 seg_5_real = glm::vec3(seg_5.x, seg_5.z, seg_5.y);
        //auto seg5_2d = glm::vec3(seg_5.x, seg_5.z, 0);
        glm::vec3 target_for_calc = target_coords;

        auto target_pl3d = map_to_xy(target_for_calc, deg_6, y_axis, seg_5);
        auto target_pl2d = glm::vec2(target_pl3d.x, target_pl3d.y);
        auto plo3d = glm::vec3(0.0f);
        auto plo2d = glm::vec2(0.0f);
        //plo2d = target_pl2d;
        auto pl3d = glm::vec3(500.0f, 0.0f, 0.0f);
        auto pl2d = glm::vec2(pl3d);
        auto s2d = glm::vec2(500);

        std::vector<segment_t*> remaining_segments;
        remaining_segments.assign(segments.begin() + 2, segments.end());
        //std::reverse(remaining_segments.begin(), remaining_segments.end());

        glm::vec2 prev_origin = target_pl2d;
        std::vector<glm::vec2> new_origins;

        if (debug_mode)
            printf("target_pl2d <%.2f,%.2f> target_pl3d <%.2f,%.2f,%.2f> target_real <%.2f,%.2f,%.2f> seg_5 <%.2f,%.2f,%.2f> deg_6: %.2f\n", target_pl2d.x, target_pl2d.y, target_pl3d.x, target_pl3d.y, target_pl3d.z, target_coords.x, target_coords.y, target_coords.z, seg_5.x, seg_5.y, seg_5.z, deg_6);

        while (true) {
            if (remaining_segments.size() < 1) {
                if (debug_mode)
                    puts("No more segments");
                break;
            }

            auto seg = remaining_segments.back();
            float segment_radius = seg->get_length();
            float total_length = 0.0f;

            for (auto *x : remaining_segments)
                total_length += x->get_length();

            float dist_to_segment = total_length - segment_radius;
            float segment_min = total_length - (segment_radius * 2);
            float dist_origin_to_prev = glm::distance<2, float>(plo2d, prev_origin);
            glm::vec2 mag = glm::normalize(prev_origin - plo2d);
            
            seg->debug_color = {0.,1.,0};
            bool skip_optim = false;

            if (remaining_segments.size() < 3) {
                if (debug_mode)
                    puts("2 or less segments left");
                skip_optim = true;
            }

            float equal_mp = ((dist_origin_to_prev * dist_origin_to_prev) -
                        (segment_radius * segment_radius) +
                        (dist_to_segment * dist_to_segment)) /
                        (2 * dist_origin_to_prev);

            float rem_dist = dist_origin_to_prev - equal_mp;
            float rem_min = -segment_radius/2.0f;
            float rem_retract = 0.0f;
            float rem_extend = segment_radius * 0.5f;
            float rem_ex2 = rem_extend * 1.75f;
            float rem_max = segment_radius * 0.95f;

            if (debug_mode)
                printf("servo: %i, rem_dist: %.2f, rem_max: %.2f, equal_mp: %.2f, segment_radius: %.2f, dist_origin_to_prev: %.2f, dist_to_segment: %.2f, total_length: %.2f, prev_origin <%.2f,%.2f>\n", seg->servo_num, rem_dist, rem_max, equal_mp, segment_radius, dist_origin_to_prev, dist_to_segment, total_length, prev_origin.x, prev_origin.y);

            auto new_origin = prev_origin;

            if (dist_to_segment < 0.05f) {
                new_origins.push_back(new_origin);
                if (debug_mode)
                    puts("Convergence");
                break;
            }

            if (rem_dist > rem_max) {
                if (debug_mode)
                    puts("Not enough overlap");
                if (debug_mode)
                    printf("rem_dist: %.2f rem_max: %.2f\n", rem_dist, rem_max);
                seg->debug_color = {0.,0.,0.};
            }

            if (!skip_optim) {
                if (segment_radius > dist_origin_to_prev) {
                    auto v = rem_extend - (segment_radius - dist_origin_to_prev);
                    equal_mp = dist_origin_to_prev - v;
                    if (debug_mode) {
                        seg->debug_color = {1.,0,0};
                        puts("Too close to origin");
                    }
                } else
                if (rem_dist < rem_extend && total_length > dist_origin_to_prev) {
                    equal_mp = dist_origin_to_prev - rem_extend;
                    if (debug_mode) {
                        seg->debug_color = {1.,1,1};
                        puts("Maintain center of gravity");
                    }
                } else
                if (rem_dist < rem_retract && total_length > dist_origin_to_prev) {
                    equal_mp = dist_origin_to_prev - rem_retract;
                    if (debug_mode) {
                        puts("Too much leftover length");
                        seg->debug_color = {1.,.5,.5};
                    }
                } else
                if (rem_dist < rem_ex2 && rem_dist >= rem_extend && total_length > dist_origin_to_prev) {
                    if (debug_mode) {
                        puts("Too much leftover length");
                        seg->debug_color = {0.,.5,.5};
                    }
                    auto r = rem_ex2 - rem_extend;
                    r = (rem_dist - rem_extend) / r;
                    auto v = r / 2.0f;

                    if (v > 0.4f)
                        v -= (v - 0.38f);

                    equal_mp = dist_origin_to_prev - (v * segment_radius + rem_extend);
                }

                if (rem_dist < rem_min) {
                    if (debug_mode)
                        puts("Too much overlap");
                        seg->debug_color = {0.25,0.25,0.25};
                    rem_dist = rem_min;
                } else
                if (rem_dist > rem_max) {
                    if (debug_mode)
                        puts("Not enough overlap");
                        seg->debug_color = {0,0,0.};
                } else {
                    rem_dist = dist_origin_to_prev - equal_mp;
                }
            }

            if (debug_mode)
                printf("rem_dist: %.2f, rem_max: %.2f, equal_mp: %.2f, dist_origin_to_prev: %.2f, dist_to_segment: %.2f\n", rem_dist, rem_max, equal_mp, dist_origin_to_prev, dist_to_segment);

            auto mp_vec = mag * equal_mp;
            //auto n = sqrtf((segment_radius - rem_dist) * (segment_radius - rem_dist));
            auto n = sqrtf(abs((segment_radius * segment_radius) - (rem_dist * rem_dist)));
            auto o = atan2(mag.y, mag.x) - (M_PI / 2.0f);
            auto new_mag = glm::normalize(glm::vec2(cosf(o),sinf(o)));
            new_origin = mp_vec + (new_mag * n);
            auto new_origin3d = glm::vec3(new_origin.x, new_origin.y, 0.0f);
            auto dist_new_prev = glm::distance<2, float>(new_origin, prev_origin);

            if (debug_mode)
                printf("mp_vec <%.2f %.2f>, segment_radius: %.2f, rem_dist: %.2f, n: %.2f, o: %.2f, new_mag <%.2f,%.2f>, dist_new_prev: %.2f\n", mp_vec[0], mp_vec[1], segment_radius, rem_dist, n, o, new_mag[0], new_mag[1], dist_new_prev);

            if (calculation_failure)
                seg->debug_color = {1.0,0,0};

            float tolerable_distance = 10.0f;

            if (abs(dist_new_prev - segment_radius) > tolerable_distance) {
                if (debug_mode)
                    puts("Distance to prev is too different");
                calculation_failure = true;
            }

            if (remaining_segments.size() < 1 && glm::distance<2, float>(new_origin, target_pl2d) > tolerable_distance) {
                if (debug_mode)
                    puts("Distance to target is too far");
                calculation_failure = true;
            }

            //if (isnot_real(new_origin.x) || isnot_real(new_origin.y))
            //    new_origin = prev_origin;//glm::vec2(0.0f);

            nocalc:;

            prev_origin = new_origin;
            new_origins.push_back(new_origin);
            remaining_segments.pop_back();
        }

        if (calculation_failure) {
            if (debug_mode)
                puts("Failed to calculate");
            return glfail;
        } else {
            if (new_origins.size() < 1) {
                if (debug_mode)
                    puts("Not enough origins");
                return glfail;
            }

            float prevrot = 0.0f;
            auto prevmag = glm::vec2(0.0f,1.0f);
            auto prev = glm::vec2(0.0f);
            new_origins.pop_back();
            std::reverse(new_origins.begin(), new_origins.end());
            new_origins.push_back(target_pl2d);

            for (int i = 0; i < new_origins.size(); i++) {
                auto cur = new_origins[i];

                auto dif = cur - prev;
                auto mag = glm::normalize(dif);

                auto servo = segments[i + 2];
                auto calcmag = mag;

                auto rot = atan2(calcmag.x, calcmag.y) - prevrot;

                auto deg = ((glm::degrees(rot)) / 180.0f) * 0.5f + 1.0f;

                if (isnot_real(deg)) {
                    deg = rot_out[i + 2];
                    rot = glm::radians(deg);
                }
                
                if (debug_mode)
                    printf("servo: %i, rot: %.2f, deg: %.2f, prevrot: %.2f, calcmag[0]: %.2f, calcmag[1]: %.2f, cur[0]: %.2f, cur[1]: %.2f, prev[0]: %.2f, prev[1]: %.2f\n", servo->servo_num, rot, deg, prevrot, calcmag.x, calcmag.y, cur.x, cur.y, prev.x, prev.y);

                
                rot_out[i + 2] = deg;

                prev = cur;
                prevmag = mag;
                prevrot = prevrot + rot;
            }
        }

        if (debug_mode)
            printf("Initial rot: %.2f,%.2f,%.2f,%.2f,%.2f\n", rot_out[0], rot_out[1], rot_out[2], rot_out[3], rot_out[4]);

        rot_out[1] = serv_6;

        if (debug_mode)
            printf("End rot: %.2f,%.2f,%.2f,%.2f,%.2f\n", rot_out[0], rot_out[1], rot_out[2], rot_out[3], rot_out[4]);
        for (int i = 0; i < segments.size(); i++)
            segments[i]->rotation = rot_out[i] * 360.0f;

        userinput_kinematic = true;
        
        slider3->set_value(s3->get_clamped_rotation());
        slider4->set_value(s4->get_clamped_rotation());
        slider5->set_value(s5->get_clamped_rotation());
        slider6->set_value(s6->get_clamped_rotation());

        return glsuccess;
    }
};

struct joystick_t {
    struct joystick_device_t {
        joystick_device_t(int jid):joystick_device_t(get_device(jid).second) {}
        joystick_device_t() {}
        
        int axis_count, button_count, jid;
        std::string guid, name, gp_name;
        std::map<int, bool> held_buttons;
        std::vector<float> deadzones;
        GLFWgamepadstate state;

        // call keys to update their state and get the updated state, stash output if need be, calling twice will ruin press detection
        int get_button(int button) {
            if (state.buttons[button]) {
                if (held_buttons[button]) {
                    return GLFW_REPEAT;
                }
                held_buttons[button] = 1;
                return GLFW_PRESS;
            }
            held_buttons[button] = 0;
            return 0;
        }

        static std::pair<std::string, joystick_device_t> get_device(int jid) {
            std::pair<std::string, joystick_device_t> kv;

            auto &jd = kv.second;
            jd.jid = jid;
            const char *guid = glfwGetJoystickGUID(jid);
            const char *gp_name = glfwGetGamepadName(jid);
            const char *name = glfwGetJoystickName(jid);

            assert(guid && "GUID null\n");

            if (!guid) 
                jd.guid = std::to_string((size_t)glfwGetJoystickUserPointer(jid));
            else
                jd.guid = guid;
            if (!gp_name) 
                jd.gp_name = "Generic";
            else
                jd.gp_name = gp_name;
            if (!name) 
                jd.name = "Joystick";
            else
                jd.name = name;

            kv.first = jd.guid;

            glfwGetJoystickAxes(jid, &jd.axis_count);
            glfwGetJoystickButtons(jid, &jd.button_count);
            
            jd.deadzones.assign(jd.axis_count, 0.0f);

            return kv;
        }
    };

    std::map<std::string, joystick_device_t> joysticks;
    
    bool pedantic_debug = false;
    bool camera_move = false;

    std::map<int, std::string> button_mapping = {
        {GLFW_GAMEPAD_BUTTON_GUIDE, "Guide"},
        {GLFW_GAMEPAD_BUTTON_LEFT_BUMPER, "L1"},
        {GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER, "R1"},
        {GLFW_GAMEPAD_BUTTON_B, "B"},
        {GLFW_GAMEPAD_BUTTON_A, "A"},
        {GLFW_GAMEPAD_BUTTON_Y, "Y"},
        {GLFW_GAMEPAD_BUTTON_X, "X"},
        {GLFW_GAMEPAD_BUTTON_START, "Start"},
        {GLFW_GAMEPAD_BUTTON_CIRCLE, "Circle"}
    };

    std::map<int, std::string> axis_mapping = {
        {GLFW_GAMEPAD_AXIS_LEFT_X, "Left X"},
        {GLFW_GAMEPAD_AXIS_LEFT_Y, "Left Y"},
        {GLFW_GAMEPAD_AXIS_RIGHT_X, "Right X"},
        {GLFW_GAMEPAD_AXIS_RIGHT_Y, "Right Y"},
        {GLFW_GAMEPAD_AXIS_LEFT_TRIGGER, "Left Trig"},
        {GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER, "Right Trig"}
    };

    void set_robot() {
        for (auto &joy : joysticks) {
            auto &guid = joy.first;
            auto &jd = joy.second;
            auto &gp = jd.state;
            auto &axes = gp.axes;
            auto &buttons = gp.buttons;
            float tolerance = 0.15;

            auto _set_v3 = [&](glm::vec3 &a, glm::vec3 &b, int i, float t) {
                if (fabs(b[i]) > t && fabs(b[i]) < 100) {
                    a[i] += b[i];
                    return true;
                }
                return false;
            };

            auto set_v3 = [&](glm::vec3 &a, glm::vec3 &b, float t) {
                return _set_v3(a, b, 0, t) | 
                       _set_v3(a, b, 1, t) | 
                       _set_v3(a, b, 2, t);
            };

            if (camera_move) {
                float pitch = -axes[3];
                float yaw = axes[2];
                if (fabs(pitch) <= tolerance)
                    pitch = 0;
                if (fabs(yaw) <= tolerance)
                    yaw = 0;
                camera->joystick_move(window, pitch, yaw * 2.0f);


                glm::vec3 jd(-axes[1], 0, axes[0]);
                if (buttons[GLFW_GAMEPAD_BUTTON_A])
                    jd.y += 0.5;
                if (buttons[GLFW_GAMEPAD_BUTTON_B])
                    jd.y -= 0.5;
                glm::vec3 ndz(0.0);
                set_v3(ndz, jd, tolerance);
                auto cyw = glm::radians(camera->yaw);
                auto vvv = glm::vec3(
                    (cos(cyw) * ndz.x) - (sin(cyw) * ndz.z),
                    ndz.y,
                    (sin(cyw) * ndz.x) + (cos(cyw) * ndz.z)
                );
                
                camera->position += vvv;
            } else {
                glm::vec3 jd(-axes[1], -axes[3], axes[0]);
                jd *= 0.25f;
                glm::vec3 ndz(0.0);
                if (!set_v3(ndz, jd, tolerance))
                    continue;

                auto cyw = glm::radians(camera->yaw);
                auto vvv = glm::vec3(
                    (cos(cyw) * ndz.x) - (sin(cyw) * ndz.z),
                    ndz.y,
                    (sin(cyw) * ndz.x) + (cos(cyw) * ndz.z)
                );
                robot_target += vvv;
                kinematics->solve_inverse(robot_target);
            }
        }
    }

    void query_robot();

    void process_input() {
        for (auto &joy : joysticks) {
            auto &jd = joy.second;
            auto &gp = jd.state;
            auto dz = jd.deadzones.data();

            if (jd.get_button(GLFW_GAMEPAD_BUTTON_GUIDE))
                memcpy(dz, &gp.axes, 4 * sizeof dz[0]);

            if (jd.get_button(GLFW_GAMEPAD_BUTTON_LEFT_BUMPER) + 
                jd.get_button(GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER) == 3)
                pedantic_debug = !pedantic_debug;

            if (jd.get_button(GLFW_GAMEPAD_BUTTON_BACK) == GLFW_PRESS)
                camera_move = !camera_move;

            if (jd.get_button(GLFW_GAMEPAD_BUTTON_START) == GLFW_PRESS)
                ::reset();

            if (jd.get_button(GLFW_GAMEPAD_BUTTON_Y) == GLFW_PRESS) {
                debug_mode = !debug_mode;
                debugInfo->hidden = !debug_mode;
                debugToggle->modified = true;
                debugToggle->toggle_state = debug_mode;
            }

            if (jd.get_button(GLFW_GAMEPAD_BUTTON_X) == GLFW_PRESS)
                query_robot();

            for (int i = 0; i < 4; i++)
                gp.axes[i] -= dz[i];
        }
    }

    void update() {
        for (auto &joy : joysticks) {
            auto &jd = joy.second;
            GLFWgamepadstate *st = &jd.state;
            if (!glfwGetGamepadState(jd.jid, st)) {
                const float *axes = glfwGetJoystickAxes(jd.jid, &jd.axis_count);
                const unsigned char *buttons = glfwGetJoystickButtons(jd.jid, &jd.button_count);

                if (jd.axis_count < 6 || jd.axis_count > 6) {
                    // redundant message, axis/button count available in the device object
                    //std::string msg = std::format("{} axes required (found {})", 6, jd.axis_count);
                    //jd.gp_name = msg;
                    if (jd.axis_count < 6)
                        continue;
                    // copy count based off GLFWgamepadstate
                    //jd.axis_count = 6;
                }

                memcpy(&st->axes, axes, sizeof st->axes);
                memcpy(&st->buttons, buttons, sizeof st->buttons);

                // remap for the controller im using
                std::swap(st->axes[2], st->axes[4]);
                std::swap(st->axes[3], st->axes[2]);
            }
        }

        process_input();
        set_robot();
    }

    std::string debug_info() {
        std::string info;
        for (auto &joy : joysticks) {
            auto &jid = joy.first;
            auto &jd = joy.second;
            info += std::format("Joystick: {}\n  Axes: {}\n  Buttons: {}\n  Jid: {}\n", jd.gp_name, jd.axis_count, jd.button_count, jd.jid);
            auto &gp = jd.state;
            for (int i = 0; i < sizeof gp.axes / sizeof gp.axes[0]; i++) {
                info += std::format("  {}: {}\n", axis_mapping[i], gp.axes[i]);
            }
            auto &jh = jd.held_buttons;
            for (int i = 0; i < sizeof gp.buttons / sizeof gp.buttons[0]; i++) {
                if (button_mapping.contains(i))
                    info += std::format("  {}: {} {}\n", button_mapping[i], gp.buttons[i], jh[i]);
                else
                if (pedantic_debug)
                    info += std::format("  {}: {} {}\n", i, gp.buttons[i], jh[i]);
            }
            /*
            const auto *buttons = glfwGetJoystickButtons(jd.jid, &jd.button_count);
            for (int i = 0; i < jd.button_count; i++)
                info += std::format("+{}: {}{}", i, buttons[i], (i+1) % 4 == 0 ? '\n' : ' ');
            info += "\n";
            */
        }   
        return info;
    }

    void remove(const std::string &guid) {
        fprintf(stderr, "Remove joystick %s\n", guid.c_str());
        joysticks.erase(guid);
    }

    void reset() {

    }

    void query_joysticks() {
        std::map<std::string, joystick_device_t> joys;
        static bool first_run = true;

        for (int jid = GLFW_JOYSTICK_1; jid < GLFW_JOYSTICK_LAST; jid++) {
            if (!glfwJoystickPresent(jid))
                continue;

            // Note, GUID can match two discrete joysticks from a single device

            auto kv = joystick_device_t::get_device(jid);
            auto &jd = kv.second;

            if (joysticks.contains(kv.first)) {
                joys.insert({kv.first, joysticks[kv.first]});
                continue;
            }

            if (kv.second.axis_count != 6) {
                if (first_run)
                    fprintf(stderr, "6 axes joystick device required, device %i \"%s\" \"%s\" \"%s\"\n", jid, jd.gp_name.c_str(), jd.name.c_str(), jd.guid.c_str());
                continue;
            }

            fprintf(stderr, "Add joystick %i \"%s\" \"%s\" \"%s\"\n", jid, jd.gp_name.c_str(), jd.name.c_str(), jd.guid.c_str());
            joys.insert(kv);
        }

        for (auto &jd : joysticks) 
            if (!joys.contains(jd.first))
                fprintf(stderr, "Remove joystick %i \"%s\" \"%s\" \"%s\"\n", jd.second.jid, jd.second.gp_name.c_str(), jd.second.name.c_str(), jd.second.guid.c_str());

        first_run = false;
        joysticks = joys;
        update();
    }
};

struct robot_interface_t {
    hid_device *handle;
    std::string serial_number;
    bool servo_sleep_on_destroy = true;

    using clk = std::chrono::high_resolution_clock;
    using tp = std::chrono::time_point<clk>;
    using dur = std::chrono::duration<long, std::milli>;

    struct robot_servo {
        robot_servo() {}
        robot_servo(int id, int min, int max, int home, int pos, int real_pos, float deg_per_second, float int_per_deg)
        :id(id),min(min),max(max),pos(pos),
        real_pos(real_pos),deg_per_second(deg_per_second),
        home(home),last_cmd(clk::now()),int_per_deg(int_per_deg) { }
        int id, min, max, home;
        // robot's real position based off a query or time-based movement interpolation
        int real_pos;
        // robot's target position, if set without last_cmd, will jerk the robot
        int pos;
        float int_per_deg, deg_per_second;
        // last time a movement was sent to the robot
        tp last_cmd;
        int min_time = 20;
        int min_diff = 2;

        float get_deg(int v) {
            return float(v - home) * int_per_deg;
        }

        int get_int(float v) {
            return (v * (1.0 / int_per_deg)) + home;
        }

        int get_elapsed_time() {
            return (int)std::chrono::duration_cast<dur>(clk::now() - last_cmd).count();
        }

        int get_interpolated_pos(bool debug = false) {
            int d = pos - real_pos;

            if (abs(d) < min_diff)
                return pos;

            dur t = std::chrono::duration_cast<dur>(clk::now() - last_cmd);
            int md = (t.count() / 1000.0f) * deg_per_second;
            int dir = d > 0 ? 1 : -1;
            int intp = dir * md;

            if (debug)
                fprintf(stderr, "gip id:%i t:%li d:%i md:%i dir:%i pos:%i intp:%i intp + real_pos:%i\n", id, t.count(), d, md, dir, pos, intp, intp + real_pos);

            if (abs(intp) > abs(d))
                return pos;

            return intp + real_pos;
        }

        bool ready_for_command() {
            return get_elapsed_time() > min_time && get_interpolated_pos() == pos;
        }

        void set_pos(int v) {
            last_cmd = clk::now();
            real_pos = pos;
            pos = v;
        }
    };

    std::map<int, robot_servo> robot_servos;

    void update() {
        if (!handle)
            return;

        std::vector servos = { s3, s4, s5, s6 };

        assert(robot_servos.size() < 7 && "Need servos to update\n");

        std::vector<std::pair<int,int>> cmds;

        static tp last_batch = clk::now();
        tp now_batch = clk::now();
        static bool constant_speed = false;
        static int u_period = 20;
        static int m_period = 2000;
        static int t_overlap = 0;
        int r_period = int(std::chrono::duration_cast<dur>(now_batch - last_batch).count()) + t_overlap;

        if (!constant_speed && r_period < u_period)
            return;

        if (!constant_speed && r_period > m_period)
            r_period = m_period;

        for (auto *sv : servos) {
            auto &rs = robot_servos[sv->servo_num];

            float targetf = sv->get_clamped_rotation();
            int targeti = rs.get_int(targetf);

            //assert(targeti >= rs.min && targeti <= rs.max && "Position out of bounds");
            if (targeti < rs.min || targeti > rs.max) {
                //fprintf(stderr, "Clamp position %i < %i < %i\n", rs.min, targeti, rs.max);
                if (targeti < rs.min)
                    targeti = rs.min;
                if (targeti > rs.max)
                    targeti = rs.max;
            }

            int initialp = rs.real_pos;
            int intrp = rs.get_interpolated_pos();
            int dist = targeti - intrp;

            if (constant_speed)
            { // constant speed
            if (abs(dist) < rs.min_diff) {
                //if (rs.id == 5)
                //    fprintf(stderr, "Skip sending command to s%i (%i - %i)\n", rs.id, intrp, targeti);
                continue;
            }
            if (!rs.ready_for_command()) {
                //if (rs.id == 6)
                //    fprintf(stderr, "Servo not ready s%i\n", rs.id);
                continue;
            }

            //rs.get_interpolated_pos(true);

            int time_to_complete = (fabs(dist) / rs.deg_per_second) * 1000;
            if (debug_mode)
                fprintf(stderr, "Send constant speed s%i (%i - (%i %.2f) = %i, %i ms)\n", rs.id, intrp, targeti, targetf, dist, time_to_complete);
            rs.set_pos(targeti);
            set_servos({{rs.id, targeti}}, time_to_complete);
            //cmds.push_back({rs.id, targeti});
            } // constant time
            else 
            {
            rs.last_cmd = now_batch;

            if (abs(dist) < rs.min_diff)
                continue;

            int mv = (r_period/1000.0f) * rs.deg_per_second;
            int mvdist = rs.real_pos - intrp;
            float accel = mvdist / (float)mv;
            float jerk = mvdist * (float)mv / (float)mv;
            int rintrp = intrp;

            if (fabs(jerk) > mv / 2) {
                intrp += (mvdist / 2); //limit jerk
            }

            //rs.set_pos(targeti);
            rs.pos = targeti;
            rs.real_pos = intrp;
            
            if (debug_mode)
                fprintf(stderr, "Send constant time s%i (%i -> %i (jerk comp %i)) (mvdist %i) (%i %.2f) = %i (%i ms %i intpersec) accel %.2f jerk %.2f\n", rs.id, initialp, rintrp, intrp, mvdist, targeti, targetf, dist, r_period, mv, accel, jerk);
            cmds.push_back({rs.id, intrp});
            }
        }

        if (!constant_speed) {
            if (cmds.size() > 0) {
                set_servos(cmds, r_period);
                last_batch = now_batch;
            }
        }
        //if (cmds.size() > 0)
        //    set_servos(cmds, 1000);
        //for (auto &cmd : cmds)
            //set_servos({cmd}, 1000);
    }

    robot_interface_t() {
        init();
    }

    ~robot_interface_t() {
        destroy();
    }

    void destroy() {
        if (handle) {
            if (servo_sleep_on_destroy)                
                servos_off();
            fprintf(stderr, "Close robot connection\n");
            close();
        }
        hid_exit();
    }

    std::string get_hid_error() {
        std::wstring err = hid_error(0);
        return std::string(err.begin(), err.end());
    }

    void read_all() {
        if (!handle)
            return;

        const unsigned char cmd[11] = {
            0x55, 0x55, 9, 21, 6, 1, 2, 3, 4, 5, 6
        };
        hid_write(handle, &cmd[0], 11);

        unsigned char ret[100];
        int count = hid_read_timeout(handle, &ret[0], 100, 2000);
        if (count < 6) {
            fprintf(stderr, "Failed to read any bytes\n%s\n", get_hid_error().c_str());
            return;
        }

        count = ret[4];

        auto now_time = clk::now();
        for (int i = 0; i < count; i++) {
            int index = 5 + 3 * i;
            int id = ret[index];
            int pos = (ret[index + 2] << 8) | ret[index + 1];
            robot_servos[id].real_pos = (unsigned short)(pos);
            robot_servos[id].last_cmd = now_time;
        }
    }

    //set no check
    void set_servo(int id, int position, int millis = 1000) {
        if (!handle)
            return;
            
        set_servos({{id,position}}, millis);
    }

    //set no check
    void set_servos(const std::vector<std::pair<int,int>> &poses, const int time = 1000) {
        if (!handle)
            return;

        const int count = poses.size() * 3 + 7;
        assert(poses.size() > 0 && "No poses");
        assert(poses.size() < 255 && count < 255 && "Should not be that big\n");
        unsigned char cmd[count];
        unsigned char header[7] = {0x55, 0x55, (unsigned char)(count - 2), 0x03, (unsigned char)(poses.size()),
            (unsigned char)(time & 0xFF), (unsigned char)(time >> 8)
        };
        memcpy(&cmd[0], &header[0], 7);

        int i = 0;
        for (auto &sv : poses) {
            int offset = i++ * 3 + 7;
            cmd[offset] = sv.first;
            cmd[offset+1] = sv.second & 0xFF;
            cmd[offset+2] = sv.second >> 8;
        }

        hid_write(handle, &cmd[0], count);
    }

    //set no check
    void servos_off() {
        unsigned char cmd[11] = { 0x55, 0x55, 9, 20, 6, 1, 2, 3, 4, 5, 6 };
        hid_write(handle, &cmd[0], 11);
        usleep(100000); //why doesn't the command work every time, im trying to fix it
    }

    void init() {
        hid_init();
        handle = nullptr;
    }

    int open(unsigned short vendor_id, unsigned short product_id, wchar_t *serial_number_w = nullptr) {
        if (handle)
            close();
        handle = hid_open(vendor_id, product_id, serial_number_w);

        if (!handle) {
            std::wstring err = hid_error(0);
            std::string rr(err.begin(), err.end());
            fprintf(stderr, "Not connected to robot, (nonfatal) reason: %s\n", rr.c_str());
            serial_number = "No USB";
            return glfail;
        }

        std::wstring wstr;
        if (!serial_number_w) {
            wchar_t wbuf[100];
            if (!hid_get_serial_number_string(handle, &wbuf[0], 100))
                wstr = std::wstring(&wbuf[0]);
            else 
                wstr = L"No serial";
        } else {
            wstr = std::wstring(serial_number_w);
        }
        serial_number = std::string(wstr.begin(), wstr.end());

        hid_set_nonblocking(handle, 0);
        fprintf(stderr, "Open robot connection\n");

        /*
        https://github.com/migsdigs/Hiwonder_xArm_ESP32
        1: 0.39 sec/60deg, 160 deg - or (60*375/90/0.39) = 641.03/s
        6-2: 0.22 sec/60deg, 240 deg - or (60*375/90/0.22) = 1136.4/s
        */

        int dmin = 200;
        int dmax = 800;
        int dhome = 500;
        int dpos = 500;
        int drp = 500;
        float dconv = 90.0f / 375.0f;
        float dps = 500.0f;
        robot_servo servos[6] = {
            { 1, 200, 850, dhome, dpos, drp, dps, dconv }, // gripper
            { 2, 50, 850, dhome, dpos, drp, dps, dconv }, // wrist
            { 3, dmin, dmax, dhome, dpos, drp, dps, dconv }, // 3
            { 4, 1, 1042, 502, dpos, drp, 700.0f, -dconv }, // 4 (inverted)
            { 5, 148, 882, 505, dpos, drp, dps, dconv }, // 5
            { 6, 1, 1146, 482, dpos, drp, 700.0f, dconv }  // base
        };

        for (int i = 0; i < 6; i++)
            robot_servos.insert({servos[i].id, servos[i]});

        read_all();

        for (auto &rs : robot_servos)
            rs.second.pos = rs.second.real_pos;

        return glsuccess;
    }

    void close() {
        if (!handle)
            return;
        robot_servos.clear();
        hid_close(handle);
        handle = nullptr;
    }

    std::string debug_info() {
        std::string ret;
        ret += std::format("USB: {}\n", serial_number);
        for (auto &rs : robot_servos) {
            ret += std::format("  {}: {}\n", rs.second.id, rs.second.real_pos);
        }
        return ret;
    }
};

void joystick_t::query_robot() {
    robot_interface->read_all();
    for (auto &rs_kv : robot_interface->robot_servos) {
        auto &rs = rs_kv.second;
        fprintf(stderr, "%i: %i (%.2f deg), ", rs.id, rs.real_pos, rs.get_deg(rs.real_pos));
    }
    fputs("\n", stderr);
}

void servo_slider_update(ui_slider_t* ui, ui_slider_t::ui_slider_v value) {
    if (userinput_kinematic)
        return;
    s6->rotation = slider6->value;
    s5->rotation = slider5->value;
    s4->rotation = slider4->value;
    s3->rotation = slider3->value;
}

void update_whatever(ui_slider_t* ui, ui_slider_t::ui_slider_v value) {
    for (int i = 0; i < slider_whatever.size(); i++) {
        float value = slider_whatever.at(i)->value;
        if (debug_mode)
            printf("%.4f%c",value, i == slider_whatever.size()-1?'\n':',');

        auto &gbl = global_text_parameters;

        switch (i) {
            case 0: gbl.scrCharacterTrimX = value; break;
            case 1: gbl.scrCharacterTrimY = value; break;
            case 2: gbl.texCharacterTrimX = value; break;
            case 3: gbl.texCharacterTrimY = value; break;
            case 4: gbl.scrCharacterSpacingScaleX = value; break;
            case 5: gbl.scrCharacterSpacingScaleY = value; break;
            case 6: gbl.scrScaleX = value; break;
            case 7: gbl.scrScaleY = value; break;
            case 8: title_height = value; break;
            case 9: value_subpos_x = value; break;
            case 10: value_subpos_y = value; break;
            case 11: camera->yaw = value; break;
            case 12: precise_factor = value; break;
            case 13: slider_frac_w = value; break;
            case 14: slider_frac_h = value; break;
            case 15: textProgram->mixFactor = value; break;
        }
    }

    ui_servo_sliders->reset();
}

void update_debug_info() {
    {
        const int bufsize = 1000;
        char char_buf[bufsize];

        glm::vec3 s3_t = s3->get_segment_vector() + s3->get_origin();
        
        debug_objects->add_sphere(robot_target, s3->model_scale);

        snprintf(char_buf, bufsize, 
        "%.0f FPS\nCamera %.2f %.2f %.2f\nFacing %.2f %.2f\nTarget %.2f %.2f %.2f\ns3 %.2f %.2f %.2f\nRotation %.2f %.2f %.2f %.2f\n%s%s",
        fps, 
        camera->position.x, camera->position.y, camera->position.z,
        camera->yaw,camera->pitch,
        robot_target.x, robot_target.y, robot_target.z,
        s3_t.x,s3_t.y,s3_t.z,
        s6->rotation, s5->rotation, s4->rotation, s3->rotation,
        joysticks->debug_info().c_str(),
        robot_interface->debug_info().c_str()
        );
        debugInfo->set_string(&char_buf[0]);
    }
}

void calcFps() {
    frameCount++;

    double nowTime = glfwGetTime();
    deltaTime = nowTime - lastTime;
    lastTime = nowTime;

    end = hrc::now();
    duration = end - start;

    if (duration.count() >= 1.0) {
        fps = frameCount / duration.count();
        frameCount = 0;
        start = hrc::now();
    }
}

void outputTest() {
   glBegin(GL_TRIANGLES);
        glColor3f(1.0f, 0.0f, 0.0f); // Red
        glVertex3f(-0.6f, -0.4f, 0.0f);
        glColor3f(0.0f, 1.0f, 0.0f); // Green
        glVertex3f(0.6f, -0.4f, 0.0f);
        glColor3f(0.0f, 0.0f, 1.0f); // Blue
        glVertex3f(0.0f, 0.6f, 0.0f);
    glEnd();
}

int init_context() {
    glfwInit();

    initial_window = {0,0,1600,900};

    window = glfwCreateWindow(initial_window[2], initial_window[3], "xArm", nullptr, nullptr);
    if (!window) {
        handle_error("failed to create glfw window");
    }
    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetJoystickCallback(joystick_callback);
    glfwSwapInterval(1);

    glfwGetWindowPos(window, &initial_window.x, &initial_window.y);
    glfwGetWindowSize(window, &initial_window[2], &initial_window[3]);

    current_window = initial_window;

    lastTime = glfwGetTime();

    signal(SIGQUIT, handle_signal);
    signal(SIGKILL, handle_signal);
    signal(SIGSTOP, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);

    return glsuccess;
}

int init() {
    camera = new camera_t();

    mainVertexShader = new shader_t("shaders/vertex.glsl", GL_VERTEX_SHADER);
    mainFragmentShader = new shader_t("shaders/fragment.glsl", GL_FRAGMENT_SHADER);
    textVertexShader = new shader_t("shaders/text_vertex_shader.glsl", GL_VERTEX_SHADER);
    textFragmentShader = new shader_t("shaders/text_fragment_shader.glsl", GL_FRAGMENT_SHADER);

    mainProgram = new shader_materials_t(shaderProgram_t(mainVertexShader, mainFragmentShader));
    textProgram = new shader_text_t(shaderProgram_t(textVertexShader, textFragmentShader));

    mainTexture = new texture_t();
    mainTexture->generate(glm::vec4(0.0f,0.0f,1.0f,1.0f));
    textTexture = new texture_t("assets/text.png");

    robotMaterial = new material_t(mainTexture,mainTexture,0.2f);
    mainProgram->set_material(robotMaterial);

    uiHandler = new ui_element_t(window, {-1.0f,-1.0f,2.0f,2.0f});
    //uiHandler->add_child(new ui_text_t(window, {0.0,0.0,.1,.1}, "Hello World!"));
    debugInfo = uiHandler->add_child(new ui_text_t(window, {-1.0f,-1.0f,2.0f,2.0f}, "", update_debug_info));
    debug_objects = new debug_object_t();
    ui_servo_sliders = uiHandler->add_child(new ui_element_t(window, uiHandler->XYWH));

    glm::vec4 sliderPos = {0.45, -0.95,0.5,0.1};
    glm::vec4 sliderAdd = {0.0,0.2,0,0};
    glm::vec2 sMM = {-180, 180.};
    
    slider6 = ui_servo_sliders->add_child(new ui_slider_t(window, sliderPos, sMM.x, sMM.y, 0., "Servo 6", true, servo_slider_update));
    slider5 = ui_servo_sliders->add_child(new ui_slider_t(window, sliderPos + (sliderAdd * glm::vec4(1.)), sMM.x, sMM.y, 0., "Servo 5", true, servo_slider_update));
    slider4 = ui_servo_sliders->add_child(new ui_slider_t(window, sliderPos + (sliderAdd * glm::vec4(2.)), sMM.x, sMM.y, 0., "Servo 4", true, servo_slider_update));
    slider3 = ui_servo_sliders->add_child(new ui_slider_t(window, sliderPos + (sliderAdd * glm::vec4(3.)), sMM.x, sMM.y, 0., "Servo 3", true, servo_slider_update));
    //debugInfo = new ui_text_t(window, {0.0,0.0,1.,1.}, "Hello world 2!");
    
    slider_ambient = debugInfo->add_child(new ui_slider_t(window, sliderPos + (sliderAdd * glm::vec4(4.)), 0., 6., 4., "Ambient Light", false));
    slider_diffuse = debugInfo->add_child(new ui_slider_t(window, sliderPos + (sliderAdd * glm::vec4(5.)), -2., 2., 0.7, "Diffuse Light", false));
    slider_specular = debugInfo->add_child(new ui_slider_t(window, sliderPos + (sliderAdd * glm::vec4(6.)), -2., 2., 0., "Specular Light", false));
    slider_shininess = debugInfo->add_child(new ui_slider_t(window, sliderPos + (sliderAdd * glm::vec4(7.)), -2., 2., 0.5, "Shininess", false));

    sliderPos = glm::vec4{-0.95,-.2,0,0} + glm::vec4{0,0,.25,0.1};
    sliderAdd = {0,.105,0,0};

    auto &gbl = global_text_parameters;
    float defaults[] = {
        gbl.scrCharacterTrimX, gbl.scrCharacterTrimY, 
        gbl.texCharacterTrimX, gbl.texCharacterTrimY, 
        gbl.scrCharacterSpacingScaleX, gbl.scrCharacterSpacingScaleY,
        gbl.scrScaleX, gbl.scrScaleX,
        title_height, value_subpos_x, value_subpos_y,
        camera->yaw, precise_factor,
        slider_frac_w, slider_frac_h,
        textProgram->mixFactor
    };

    bool extra_slider_hidden = true;
    for (int i = 0; i < (sizeof defaults / sizeof defaults[0]); i++) {
        int stepover = 8;
        if (i == stepover)
            sliderPos += glm::vec4(sliderPos[2]+0.1,0,0,0);
        slider_whatever.push_back(debugInfo->add_child(new ui_slider_t(window, sliderPos + (sliderAdd * glm::vec4(float(i % stepover))), defaults[i]-2., defaults[i]+2., defaults[i], "", false, update_whatever, false, true)));
    }

    debugInfo->hidden = !debug_mode;
    debugToggle = uiHandler->add_child(new ui_toggle_t(window, {-.975,.925,.05,.05}, "Debug", debug_mode, [](ui_toggle_t* ui, bool state){
        debug_mode = state;
        debugInfo->hidden = !debug_mode;
    }));
    debugInfo->add_child(new ui_toggle_t(window, {-.865, .925,.05,.05}, "Reset", false, [](ui_toggle_t *ui, bool state) {
        if (state)
            uiHandler->reset();
        reset();
    }));

    sBase = new segment_t(nullptr, z_axis, z_axis, 46.19, 7);
    s6 = new segment_t(sBase, z_axis, z_axis, 35.98, 6);
    s5 = new segment_t(s6, y_axis, z_axis, 96.0, 5);
    s4 = new segment_t(s5, y_axis, z_axis, 96.0, 4);
    s3 = new segment_t(s4, y_axis, z_axis, 150.0, 3);

    segments = std::vector<segment_t*>({sBase, s6, s5, s4, s3});
    kinematics = new kinematics_t();

    joysticks = new joystick_t;
    robot_interface = new robot_interface_t;

    return glsuccess;
}

void reset() {
    camera = new (camera)camera_t;
    camera->position = {-30,20,0.};
    camera->yaw = 0.001;
    camera->pitch = 0.001;
    for (auto &seg : segments)
        seg->rotation = 0;
    robot_target = s3->get_segment_vector() + s3->get_origin();
}

int load() {
    if ((textTexture->load() ||
        mainProgram->load() ||
        textProgram->load())) {
        handle_error("a component failed to load");
    }

    if (sBase->load("assets/xarm-sbase.stl") ||
        s6->load("assets/xarm-s6.stl") ||
        s5->load("assets/xarm-s5.stl") ||
        s4->load("assets/xarm-s4.stl") ||
        s3->load("assets/xarm-s3.stl")) {
        handle_error("a model failed to load");
    }

    reset();
    uiHandler->load();
    //debugInfo->load();

    joysticks->query_joysticks();
    robot_interface->open(1155, 22352);
    robot_interface->read_all();

    return glsuccess;
}

int main() {
    if (init_context() || init() || load())
        handle_error("failed to load", glfail);

    while (!glfwWindowShouldClose(window)) {
        calcFps();

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        handle_keyboard(window, deltaTime);
        joysticks->update();
        robot_interface->update();

        mainProgram->use();
        mainProgram->set_camera(camera);

        //glm::vec3 diffuse(0.6f), specular(0.2f), ambient(0.0f);
        glm::vec3 diffuse(slider_diffuse->value), specular(slider_specular->value), ambient(slider_ambient->value);

        float shininess = slider_shininess->value;
        robotMaterial->shininess = shininess;

        mainProgram->set_v3("eyePos", camera->position);
        mainProgram->set_v3("light.position", glm::vec3(5.0f, 15.0f, 5.0f));
        mainProgram->set_v3("light.ambient", ambient);
        mainProgram->set_v3("light.diffuse", diffuse);
        mainProgram->set_v3("light.specular", specular);
        //mainProgram->set_f("material.shininess", 2.0f);
        mainProgram->set_f("material.shininess", shininess);

        mainProgram->set_material(robotMaterial);

        for (auto *segment : segments)
            segment->render();

        if (debug_mode)
            debug_objects->render();    
        
        if (uiHandler->render())
            break;

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    safe_exit(0);
}

void destroy() {
    glfwTerminate();
    robot_interface->destroy();
}

void hint_exit() {
    glfwSetWindowShouldClose(window, 1);
}

void safe_exit(int errcode) {
    destroy();
    exit(errcode);
}

void handle_error(const char *str, int errcode) {
    fprintf(stderr, "Error: %s\n", str);
    safe_exit(errcode);
}

void handle_signal(int sig) {
    fprintf(stderr, "Close signal caught\n");
    //safe_exit();
    hint_exit();
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    current_window[2] = width;
    current_window[3] = height;
    glViewport(0, 0, width, height);
    uiHandler->onFramebuffer(width, height);
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
    if (uiHandler->onMouse(button, action, mods))
        return;

    camera->mousePress(window, button, action, mods);
}

void cursor_position_callback(GLFWwindow *window, double x, double y) {
    if (uiHandler->onCursor(x, y))
        return;

    camera->mouseMove(window, x, y);
}

void handle_keyboard(GLFWwindow* window, float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    static tp last_toggle = hrc::now();
    std::chrono::duration<float> duration = hrc::now() - last_toggle;

    if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS && duration.count() > .5) {
        fullscreen = !fullscreen;

        GLFWmonitor *monitor = glfwGetWindowMonitor(window);
        if (!monitor)
            monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode *mode = glfwGetVideoMode(monitor);

        if (fullscreen)
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        else
            glfwSetWindowMonitor(window, 0, initial_window[0], initial_window[1], initial_window[2], initial_window[3], mode->refreshRate);

        last_toggle = hrc::now();
    }

    if (uiHandler->onKeyboard(deltaTime))
        return;

    static bool x_press = false;
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
        if (!x_press)
            joysticks->query_robot();
        x_press = true;
    } else {
        x_press = false;
    }

    camera->keyboard(window, deltaTime);

    int raise[] = {GLFW_KEY_R, GLFW_KEY_T, GLFW_KEY_Y, GLFW_KEY_U};
    int lower[] = {GLFW_KEY_F, GLFW_KEY_G, GLFW_KEY_H, GLFW_KEY_J};
    segment_t *segments[] = {s6,s5,s4,s3};

    float movementFactor = movementSpeed;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        movementFactor = rapidSpeed;

    bool change_2 = false;

    for (int i = 0; i < 4; i++) {
        if (glfwGetKey(window, raise[i]) == GLFW_PRESS) {
            segments[i]->rotation += movementFactor * deltaTime;
            change_2 = true;
        }

        if (glfwGetKey(window, lower[i]) == GLFW_PRESS) {
            segments[i]->rotation -= movementFactor * deltaTime;
            change_2 = true;
        }
    }

    if (change_2)
        robot_target = s3->get_segment_vector() + s3->get_origin();

    int robot3d[] = {GLFW_KEY_O, GLFW_KEY_L, GLFW_KEY_K, GLFW_KEY_SEMICOLON, GLFW_KEY_I, GLFW_KEY_P};
    bool robot3d_o[6];
    bool change = false;

    for (int i = 0; i < 6; i++) {
        robot3d_o[i] = (glfwGetKey(window, robot3d[i]) == GLFW_PRESS);

        if (robot3d_o[i]) {
            change = true;

            switch (i) {
                case 0:
                    robot_target.x += movementFactor * deltaTime;
                    break;
                case 1:
                    robot_target.x -= movementFactor * deltaTime;
                    break;
                case 2:
                    robot_target.z -= movementFactor * deltaTime;
                    break;
                case 3:
                    robot_target.z += movementFactor * deltaTime;
                    break;
                case 4:
                    robot_target.y -= movementFactor * deltaTime;
                    break;
                case 5:
                    robot_target.y += movementFactor * deltaTime;
                    break;
            }
        }
    }

    if (change) {
        kinematics->solve_inverse(robot_target);
    }
}

void joystick_callback(int jid, int event) {
    //printf("%i, %i\n", jid, event);
    joysticks->query_joysticks();
}