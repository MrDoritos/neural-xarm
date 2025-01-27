#include <iostream>
#include <limits>
#include <chrono>

#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <GL/gl.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "thirdparty/stb_image.h"
#include "thirdparty/stl_reader.h"

struct camera_t;
struct mesh_t;
struct texture_t;
struct shader_t;
struct shaderProgram_t;
struct segment_t;
struct kinematics_t;
struct debug_info_t;

int width = 1600, height = 900;
float lastTime;
float mouseSensitivity = 0.05f;
float preciseSpeed = 0.1f;
float movementSpeed = 10.0f;
float rapidSpeed = 100.0f;
GLFWwindow *window;
GLint uni_projection, uni_model, uni_norm, uni_view;
const GLuint gluninitialized = -1, glfail = -1, glsuccess = GL_NO_ERROR;
texture_t *textTexture;
shader_t *mainVertexShader, *mainFragmentShader;
shader_t *textVertexShader, *textFragmentShader;
shaderProgram_t *mainProgram, *textProgram;
camera_t *camera;
segment_t *sBase, *s6, *s5, *s4, *s3;
std::vector<segment_t*> segments;
debug_info_t *debugInfo;
kinematics_t *kinematics;
glm::vec3 robot_target(0.0f);

using hrc = std::chrono::high_resolution_clock;
using tp = std::chrono::time_point<hrc>;
tp start, end;
int frameCount = 0;
float fps;

glm::vec4 x_axis(1,0,0,0), y_axis(0,1,0,0), z_axis(0,0,1,0);

struct camera_t {
    camera_t() {
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
    }

    glm::vec3 position, up, front, right;
    float fov, near, far, pitch, yaw;
    double last_mouse[2];
    bool no_mouse = true;

    glm::mat4 getViewMatrix() {
        return glm::lookAt(position, position + front, up);
    }

    void mouseMove(GLFWwindow *window, double x, double y) {
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

        front = glm::normalize(glm::vec3(
            std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch)),
            std::sin(glm::radians(pitch)),
            std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch))
        ));

        right = glm::cross(front, up);

        //printf("LookAt: yaw: %f pitch: %f\n", yaw, pitch);
    }

    void keyboard(GLFWwindow *window, float deltaTime) {
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
    std::vector<float> coords, normals;
    std::vector<unsigned int> tris, solids;
    GLuint vao, vbo, vertexCount;
    glm::vec3 minBound, maxBound;
    glm::vec3 position;
    
    mesh_t() {
        position = glm::vec3(0.0f);
    }

    struct _vnt {
        glm::vec3 vertex;
        glm::vec3 normal;
        glm::vec2 tex;
    };

    bool load(const char *filename) {
        stl_reader::ReadStlFile(filename, coords, normals, tris, solids);
        
        if (tris.size() < 1) {
            return glfail;
        }

        minBound = glm::vec3(std::numeric_limits<float>().max());
        maxBound = glm::vec3(std::numeric_limits<float>().min());

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        vertexCount = tris.size();
        int triCount = vertexCount / 3;

        _vnt *verticies = new _vnt[vertexCount];

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
                }
            }

            for (int i = 0; i < vertexCount; i++) {
                auto vertex = verticies[i].vertex;
                //printf("x: %f, y: %f, z: %f\n", vertex.x, vertex.y, vertex.z);
                minBound.x = std::min(minBound.x, verticies[i].vertex.x);
                minBound.y = std::min(minBound.y, verticies[i].vertex.y);
                minBound.z = std::min(minBound.z, verticies[i].vertex.z);
                maxBound.x = std::max(maxBound.x, verticies[i].vertex.x);
                maxBound.y = std::max(maxBound.y, verticies[i].vertex.y);
                maxBound.z = std::max(maxBound.z, verticies[i].vertex.z);
            }

            size_t coordSize = sizeof verticies[0].vertex;
            size_t normSize = sizeof verticies[0].normal;
            size_t texSize = sizeof verticies[0].tex;

            size_t stride = sizeof verticies[0];


            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, vertexCount * stride, verticies, GL_STATIC_DRAW);
            
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*) (0));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*) (normSize));
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*) (normSize + texSize));

            glBindVertexArray(0);

            printf("Uploaded %i verticies, stride: %li, size: %li, vbo: %i, addr: %p\n", vertexCount, stride, vertexCount * sizeof verticies[0], vbo, verticies);
            printf("Min: x: %f, y: %f, z: %f\n", minBound.x, minBound.y, minBound.z);
            printf("Max: x: %f, y: %f, z: %f\n", maxBound.x, maxBound.y, maxBound.z);
        }

        delete [] verticies;

        return glsuccess;
    }

    void render() {
        //printf("Draw %i with %i verticies\n", vbo, vertexCount);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        glBindVertexArray(0);
    }
};

struct texture_t {
    GLuint textureId;
    std::string path;

    texture_t(std::string path)
    :path(path),textureId(gluninitialized) { }

    bool isLoaded() {
        return textureId != gluninitialized;
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

    template<typename ...Ts>
    shaderProgram_t(Ts ...shaders)
    :shaders({shaders...}),programId(gluninitialized) { }

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

        return glsuccess;
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
        return glm::normalize(mat[2]);
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
            auto trn = glm::vec3(2.54f * model_scale, 0., 0.);
            iden = glm::translate(iden, trn);
            iden = glm::rotate(iden, glm::radians(90.0f), {0,0,1});

            return parent->get_segment_vector() + 
                   parent->get_origin() + 
                   glm::vec3(iden[3]);
        }

        return parent->get_segment_vector() + parent->get_origin();
    }

    void renderVector(glm::vec3 origin, glm::vec3 end) {
        glBegin(GL_LINES);
            glColor3f(1.0,0,0);
            glVertex3f(origin.x,origin.y,origin.z);
            glVertex3f(end.x,end.y,end.z);
        glEnd();
    }

    void renderMatrix(glm::vec3 origin, glm::mat4 mat) {
        glm::vec4 m = glm::vec4(origin.x, origin.y, origin.z, 0);

        renderVector(origin, m + mat[0]);
        renderVector(origin, m + mat[1]);
        renderVector(origin, m + mat[2]);
    }

    void renderDebug() {
        glm::vec3 origin = get_origin();
        renderVector(origin, get_origin());
        renderVector(origin, get_segment_vector());

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
        glUseProgram(mainProgram->programId);
        glm::mat4 model = glm::mat4(1.);
        glUniformMatrix4fv(uni_model, 1, GL_FALSE, glm::value_ptr(model));
        renderDebug();
        model = get_model_transform();
        glUniformMatrix4fv(uni_model, 1, GL_FALSE, glm::value_ptr(model));
        mesh_t::render();
    }

    float model_scale = 0.1;
    glm::vec3 direction, rotation_axis, initial_direction;
    segment_t *parent;
    float length, rotation;
    int servo_num;
};

struct debug_info_t {
    std::string string_buffer;
    GLuint vbo, vao, vertexCount;
    int currentX, currentY;
    float screenX, screenY;
    bool modified;

    struct text_t {
        float x, y, u, v;
        text_t(float x, float y, float u, float v)
        :x(x),y(y),u(u),v(v) { }
        text_t() { }
    };

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

    debug_info_t() {
        reset();
    }

    void reset() {
        string_buffer.clear();
        currentX = currentY = 0;
        screenX = screenY = 0;
        vertexCount = 0;
        modified = true;
    }

    bool load() {
        glGenBuffers(1, &vbo);
        glGenVertexArrays(1, &vao);

        return glsuccess;
    }

    void render() {
        if (modified)
            mesh();

        glDisable(GL_CULL_FACE);
        glUseProgram(textProgram->programId);
        glBindTexture(GL_TEXTURE_2D, textTexture->textureId);
        glBindVertexArray(vao);
        glm::mat4 model = glm::translate(glm::mat4(1.0), glm::vec3(0.0));
        glUniformMatrix4fv(uni_model, 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        glBindVertexArray(0);
    }

    protected:
    void add_character(text_t *buffer, int character) {
        //const float fontSize = 1.0f / 16.0f;
        const float fontWidth = 1.0f / 16.0f,
                    fontHeight = 1.0f / 16.0f;
        const float screenWidth = fontWidth / 2, 
                    screenHeight = fontHeight;
        float sX = screenX - 1.0f;
        float sY = screenY - (1.0f - screenHeight);
        float sW = screenWidth;
        float sH = screenHeight;
        float fW = fontWidth;
        float fH = fontHeight;


        const text_t verticies[6] = {
            {sX,sY,0,0},
            {sX+sW,sY,fW,0},
            {sX,sY+sH,0,fH},
            {sX+sW,sY,fW,0},
            {sX+sW,sY+sH,fW,fH},
            {sX,sY+sH,0,fH}
        };

        float textX = (character % 16) * fontWidth;
        float textY = (character / 16 % 16) * fontHeight;
        float posX = currentX * screenWidth;
        float posY = currentY * screenHeight;

        for (int i = 0; i < 6; i++) {
            buffer[vertexCount].x = verticies[i].x + posX;
            buffer[vertexCount].y = -verticies[i].y + (screenHeight - posY);
            buffer[vertexCount].u = verticies[i].u + textX;
            buffer[vertexCount].v = verticies[i].v + textY + (1.0f / 256.0f); 

            vertexCount++;
        }
    }

    void mesh() {
        currentX = currentY = vertexCount = 0;

        if (string_buffer.size() < 1) {
            modified = false;
            return;
        }

        text_t *buffer = new text_t[string_buffer.size() * 6];

        for (auto ch : string_buffer) {
            if (ch == '\n') {
                currentX = 0;
                currentY++;
                continue;
            }

            add_character(buffer, ch);
            currentX++;
        }

        modified = false;

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof * buffer, buffer, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 16, (const void*) (0));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 16, (const void*) (8));
        glEnableVertexAttribArray(1);

        //printf("Text buffer: %i verticies\n", vertexCount);

        delete [] buffer;
    }
};

struct kinematics_t {
    static glm::vec2 map_to_xy(glm::vec3 pos, float r = 0.0f, glm::vec3 a = -z_axis, glm::vec3 o = glm::vec3(0.0f), glm::vec3 t = glm::vec3(0.0f)) {
        //auto mapped = segment_t::rotate_vector_3d(pos - o, a, r);
        auto matrix = glm::translate(glm::mat4(1.0f), pos - o);
        matrix = glm::rotate(matrix, glm::radians(r), a);
        auto npos = glm::vec3(matrix[3].x, matrix[3].z, 0);
        return npos + t;
    }

    bool solve_inverse(glm::vec3 coordsIn) {
        auto isnot_real = [](float x){
            return (isinf(x) || isnan(x));
        };

        bool calculation_failure = false;

        auto target_coords = coordsIn;
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

        auto target_pl3d = map_to_xy({target_coords.x, target_coords.z, target_coords.y}, deg_6, -z_axis, seg_5_real);
        auto target_pl2d = glm::vec2(target_pl3d);
        auto plo3d = glm::vec3(0.0f);
        auto plo2d = glm::vec2(0.0f);
        auto pl3d = glm::vec3(500.0f, 0.0f, 0.0f);
        auto pl2d = glm::vec2(pl3d);
        auto s2d = glm::vec2(500);

        std::vector<segment_t*> remaining_segments;
        remaining_segments.assign(segments.begin() + 2, segments.end());
        //std::reverse(remaining_segments.begin(), remaining_segments.end());

        auto prev_origin = target_pl2d;
        std::vector<glm::vec2> new_origins;

        printf("target_pl2d <%.2f,%.2f> target_pl3d <%.2f,%.2f,%.2f> seg_5 <%.2f,%.2f,%.2f> deg_6: %.2f\n", target_pl2d.x, target_pl2d.y, target_coords.x, target_coords.y, target_coords.z, seg_5.x, seg_5.y, seg_5.z, deg_6);

        while (true) {
            if (remaining_segments.size() < 1) {
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
            auto mag = glm::normalize(prev_origin - plo2d);
            
            bool skip_optim = false;

            if (remaining_segments.size() < 3) {
                puts("2 or less segments left");
                skip_optim = true;
            }

            float equal_mp = ((dist_origin_to_prev * dist_origin_to_prev) -
                        (segment_radius * segment_radius) +
                        (dist_to_segment * dist_to_segment)) /
                        (dist_origin_to_prev * dist_origin_to_prev);
            float rem_dist = dist_origin_to_prev - equal_mp;
            float rem_min = -segment_radius/2.0f;
            float rem_retract = 0.0f;
            float rem_extend = segment_radius * 0.5f;
            float rem_ex2 = rem_extend * 1.75f;
            float rem_max = segment_radius * 0.95f;

            printf("servo: %i, rem_dist: %.2f, rem_max: %.2f, equal_mp: %.2f, segment_radius: %.2f, dist_origin_to_prev: %.2f, dist_to_segment: %.2f, total_length: %.2f, prev_origin <%.2f,%.2f>\n", seg->servo_num, rem_dist, rem_max, equal_mp, segment_radius, dist_origin_to_prev, dist_to_segment, total_length, prev_origin.x, prev_origin.y);

            auto new_origin = prev_origin;

            if (dist_to_segment < 0.05f) {
                new_origins.push_back(new_origin);
                puts("Convergence");
                break;
            }

            if (rem_dist > rem_max) {
                puts("Not enough overlap");
                printf("rem_dist: %.2f rem_max: %.2f\n", rem_dist, rem_max);
            }

            if (!skip_optim) {
                if (segment_radius > dist_origin_to_prev) {
                    auto v = rem_extend - (segment_radius - dist_origin_to_prev);
                    equal_mp = dist_origin_to_prev - v;
                } else
                if (rem_dist < rem_extend && total_length > dist_origin_to_prev) {
                    equal_mp = dist_origin_to_prev - rem_extend;
                } else
                if (rem_dist < rem_retract && total_length > dist_origin_to_prev) {
                    equal_mp = dist_origin_to_prev - rem_retract;
                } else
                if (rem_dist < rem_ex2 && rem_dist >= rem_extend && total_length > dist_origin_to_prev) {
                    auto r = rem_ex2 - rem_extend;
                    r = (rem_dist - rem_extend) / r;
                    auto v = r / 2.0f;

                    if (v > 0.4f)
                        v -= (v - 0.38f);

                    equal_mp = dist_origin_to_prev - (v * segment_radius + rem_extend);
                }

                if (rem_dist < rem_min) {
                    puts("Too much overlap");
                    rem_dist = rem_min;
                } else
                if (rem_dist > rem_max) {
                    puts("Not enough overlap");
                } else {
                    rem_dist = dist_origin_to_prev - equal_mp;
                }
            }

            printf("rem_dist: %.2f, rem_max: %.2f, equal_mp: %.2f, dist_origin_to_prev: %.2f, dist_to_segment: %.2f\n", rem_dist, rem_max, equal_mp, dist_origin_to_prev, dist_to_segment);

            auto mp_vec = mag * equal_mp;
            auto n = sqrtf((segment_radius * segment_radius) - (rem_dist * rem_dist));
            auto o = atan2(mag.y, mag.x) - (M_PI / 2.0f);
            auto new_mag = glm::vec2(cosf(o),sinf(o));
            new_origin = mp_vec + (new_mag * n);
            auto new_origin3d = glm::vec3(new_origin.x, new_origin.y, 0.0f);
            auto dist_new_prev = glm::distance<2, float>(new_origin, prev_origin);

            float tolerable_distance = 10.0f;

            if (abs(dist_new_prev - segment_radius) > tolerable_distance) {
                puts("Distance to prev is too different");
                calculation_failure = true;
            }

            if (remaining_segments.size() < 1 && glm::distance<2, float>(new_origin, target_pl2d) > tolerable_distance) {
                puts("Distance to target is too far");
                calculation_failure = true;
            }

            if (isnot_real(new_origin.x) || isnot_real(new_origin.y))
                new_origin = prev_origin;//glm::vec2(0.0f);

            nocalc:;

            prev_origin = new_origin;
            new_origins.push_back(new_origin);
            remaining_segments.pop_back();
        }

        if (calculation_failure) {
            puts("Failed to calculate");
            return glfail;
        } else {
            if (new_origins.size() < 1) {
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
                
                printf("servo: %i, rot: %.2f, deg: %.2f, prevrot: %.2f, calcmag[0]: %.2f, calcmag[1]: %.2f, cur[0]: %.2f, cur[1]: %.2f, prev[0]: %.2f, prev[1]: %.2f\n", servo->servo_num, rot, deg, prevrot, calcmag.x, calcmag.y, cur.x, cur.y, prev.x, prev.y);

                
                rot_out[i + 2] = deg;

                prev = cur;
                prevmag = mag;
                prevrot += rot;
            }
        }

        printf("Initial rot: %.2f,%.2f,%.2f,%.2f,%.2f\n", rot_out[0], rot_out[1], rot_out[2], rot_out[3], rot_out[4]);

        rot_out[1] = serv_6;

        printf("End rot: %.2f,%.2f,%.2f,%.2f,%.2f\n", rot_out[0], rot_out[1], rot_out[2], rot_out[3], rot_out[4]);
        for (int i = 0; i < segments.size(); i++)
            segments[i]->rotation = rot_out[i] * 360.0f;
        return glsuccess;
    }
};

void init() {
    camera = new camera_t();
    textTexture = new texture_t("assets/text.png");

    mainVertexShader = new shader_t("shaders/vertex.glsl", GL_VERTEX_SHADER);
    mainFragmentShader = new shader_t("shaders/fragment.glsl", GL_FRAGMENT_SHADER);
    textVertexShader = new shader_t("shaders/text_vertex_shader.glsl", GL_VERTEX_SHADER);
    textFragmentShader = new shader_t("shaders/text_fragment_shader.glsl", GL_FRAGMENT_SHADER);

    mainProgram = new shaderProgram_t(mainVertexShader, mainFragmentShader);
    textProgram = new shaderProgram_t(textVertexShader, textFragmentShader);

    debugInfo = new debug_info_t();

    sBase = new segment_t(nullptr, z_axis, z_axis, 46.19, 7);
    s6 = new segment_t(sBase, z_axis, z_axis, 35.98, 6);
    s5 = new segment_t(s6, y_axis, z_axis, 96.0, 5);
    s4 = new segment_t(s5, y_axis, z_axis, 96.0, 4);
    s3 = new segment_t(s4, y_axis, z_axis, 150.0, 3);

    segments = std::vector<segment_t*>({sBase, s6, s5, s4, s3});
    kinematics = new kinematics_t();
}

void load() {
    if ((textTexture->load() ||
        mainProgram->load() ||
        textProgram->load() ||
        debugInfo->load())) {
        std::cout << "A component failed to load" << std::endl;
        glfwTerminate();
        exit(-1);
    }

    if (sBase->load("assets/xarm-sbase.stl") ||
        s6->load("assets/xarm-s6.stl") ||
        s5->load("assets/xarm-s5.stl") ||
        s4->load("assets/xarm-s4.stl") ||
        s3->load("assets/xarm-s3.stl")) {
        std::cout << "A model failed to load" << std::endl;
        glfwTerminate();
        exit(-1);
    }

    robot_target = s3->get_segment_vector() + s3->get_origin();
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void cursor_position_callback(GLFWwindow *window, double x, double y);
void handle_keyboard(GLFWwindow* window, float deltaTime);

void calcFps() {
    frameCount++;
    end = hrc::now();
    std::chrono::duration<float> duration = end - start;

    if (duration.count() >= 1.0) {
        fps = frameCount / duration.count();
        frameCount = 0;
        start = hrc::now();
    }
}

void renderDebugInfo() {
    {
        const int bufsize = 1000;
        char char_buf[bufsize];

        glm::vec3 s3_t = s3->get_segment_vector() + s3->get_origin();
        
        snprintf(char_buf, bufsize, 
        "FPS: %.0f\nCamera<%.2f,%.2f,%.2f>\nTarget<%.2f,%.2f,%.2f>\ns3<%.2f,%.2f,%.2f>\nRotation<%.2f,%.2f,%.2f,%.2f>",
        fps, 
        camera->position.x, camera->position.y, camera->position.z,
        robot_target.x, robot_target.y, robot_target.z,
        s3_t.x,s3_t.y,s3_t.z,
        s6->rotation, s5->rotation, s4->rotation, s3->rotation
        );
        debugInfo->set_string(&char_buf[0]);
    }

    debugInfo->render();
}

int main() {
    glfwInit();
    window = glfwCreateWindow(width, height, "xArm", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSwapInterval(1);

    lastTime = glfwGetTime();

    init();

    load();

    uni_model = glGetUniformLocation(mainProgram->programId, "model");
    uni_view = glGetUniformLocation(mainProgram->programId, "view");
    uni_projection = glGetUniformLocation(mainProgram->programId, "projection");
    uni_norm = glGetUniformLocation(mainProgram->programId, "norm");

    //glViewport(0, 0, width, height);
    while (!glfwWindowShouldClose(window)) {
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float nowTime = glfwGetTime();
        float deltaTime = nowTime - lastTime;
        lastTime = nowTime;
        handle_keyboard(window, deltaTime);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glDisable(GL_CULL_FACE);
        glEnable(GL_ALPHA_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glm::mat4 model = glm::scale(glm::mat4(1.0), glm::vec3(0.1));
        //glm::mat4 model = glm::translate(glm::mat4(1.0), glm::vec3(0));
        glm::mat4 view = camera->getViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(camera->fov),
                                                (float) width / height, camera->near, camera->far);
        glm::mat3 norm(model);
        norm = glm::inverse(norm);
        norm = glm::transpose(norm);

        glUseProgram(mainProgram->programId);

        glUniformMatrix4fv(uni_model, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(uni_view, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(uni_projection, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(uni_norm, 1, glm::value_ptr(norm));

        /*
        glBegin(GL_TRIANGLES);
            glColor3f(1.0f, 0.0f, 0.0f); // Red
            glVertex3f(-0.6f, -0.4f, 0.0f);
            glColor3f(0.0f, 1.0f, 0.0f); // Green
            glVertex3f(0.6f, -0.4f, 0.0f);
            glColor3f(0.0f, 0.0f, 1.0f); // Blue
            glVertex3f(0.0f, 0.6f, 0.0f);
        glEnd();
        */

        for (auto *segment : segments)
            segment->render();

        calcFps();

        renderDebugInfo();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    ::width = width;
    ::height = height;
    glViewport(0, 0, width, height);
}

void cursor_position_callback(GLFWwindow *window, double x, double y) {
    camera->mouseMove(window, x, y);
}

void handle_keyboard(GLFWwindow* window, float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
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
                    robot_target.z += movementFactor * deltaTime;
                    break;
                case 1:
                    robot_target.z -= movementFactor * deltaTime;
                    break;
                case 2:
                    robot_target.x -= movementFactor * deltaTime;
                    break;
                case 3:
                    robot_target.x += movementFactor * deltaTime;
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
