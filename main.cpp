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
debug_info_t *debugInfo;

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
    
    struct _vnt {
        glm::vec3 vertex;
        glm::vec3 normal;
        glm::vec2 tex;
    };

    void load(const char *filename) {
        stl_reader::ReadStlFile(filename, coords, normals, tris, solids);
        
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

    glm::vec3 matrix_to_vector(glm::mat4 mat) {
        glm::vec3 sum;
        for (int i = 0; i < 3; i++)
            sum += glm::vec3(mat[i]);
        return glm::normalize(mat[2]);
    }

    float get_length() {
        return length * model_scale;
    }

    glm::mat4 get_base_matrix() {
        return glm::mat4(x_axis, y_axis, z_axis, glm::vec4(0,0,0,1));
    }

    glm::mat4 vector_to_matrix(glm::vec3 vec) {
        glm::mat4 out;
        glm::vec4 inv(vec.x, vec.y, vec.z, 0.0);
        out[0] = inv * x_axis;
        out[1] = inv * y_axis;
        out[2] = inv * z_axis;
        out[3] = glm::vec4(0,0,0,1);
        return out;
    }

    glm::vec3 rotate_vector_3d(glm::vec3 vec, glm::vec3 axis, float degrees) {
        float radians = glm::radians(degrees);
        glm::mat4 rtn_matrix = vector_to_matrix(vec);

        return matrix_to_vector(glm::rotate(rtn_matrix, radians, axis));
    }

    glm::mat4 get_rotation_matrix() {
        //glm::mat4 base_rotation(x_axis, y_axis, z_axis, glm::vec4(0,0,0,1));
        glm::mat4 base_rotation(1.0f);

        if (!parent) {
            return glm::rotate(base_rotation, glm::radians(-90.0f), glm::vec3(x_axis));
            //return base_rotation;
        }

        glm::mat4 parent_matrix = parent->get_rotation_matrix();
        glm::mat4 rotation_axis_matrix = glm::rotate(parent_matrix, glm::radians(rotation), rotation_axis);
        
        return rotation_axis_matrix;

        for (int i = 0; i < 3; i++)
            rotation_axis_matrix[i] = parent_matrix[i] * rotation_axis[i];

        glm::vec3 rotation_axis_vector = matrix_to_vector(rotation_axis_matrix);

        glm::mat4 rotated_axis = parent_matrix * rotation_axis_matrix;

        return glm::rotate(parent_matrix, rotation, rotation_axis_vector);

        //return parent_matrix;
    }

    glm::mat4 get_model_transform() {
        model_scale = 1.0;
        rotation += 0.1f;
        glm::vec3 origin = get_origin();
        glm::mat4 matrix(1.);
        //matrix = glm::rotate(matrix, glm::radians(-90.0f), glm::vec3(x_axis));
        //matrix = glm::rotate(matrix, glm::radians(rotation), glm::vec3(rotation_axis));
        matrix = glm::translate(matrix, origin);
        matrix *= get_rotation_matrix();
        matrix = glm::scale(matrix, glm::vec3(model_scale));
        //std::cout << glm::to_string(matrix) << std::endl;
        //std::cout << glm::to_string(get_rotation_matrix()) << std::endl;
        return matrix;        
    }

    glm::vec3 get_segment_vector() {
        /*
        if (servo_num == 7)
            return glm::vec3(2.54,0,get_length());
        */
        return matrix_to_vector(get_rotation_matrix()) * get_length();
    }

    glm::vec3 get_origin() {
        if (!parent)
            return position;
/*
        if (servo_num == 5) {
            return parent->get_segment_vector() + 
                   parent->get_origin() + 
                   (rotate_vector_3d(get_rotation_matrix()[1], z_axis, 90.0) * glm::vec3(2.54 * model_scale));
        }*/

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
        std::cout << glm::to_string(origin) << std::endl;
        std::cout << glm::to_string(mat) << std::endl;

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

void init() {
    camera = new camera_t();
    textTexture = new texture_t("text.png");

    mainVertexShader = new shader_t("shader/vertex.glsl", GL_VERTEX_SHADER);
    mainFragmentShader = new shader_t("shader/fragment.glsl", GL_FRAGMENT_SHADER);
    textVertexShader = new shader_t("shader/text_vertex_shader.glsl", GL_VERTEX_SHADER);
    textFragmentShader = new shader_t("shader/text_fragment_shader.glsl", GL_FRAGMENT_SHADER);

    mainProgram = new shaderProgram_t(mainVertexShader, mainFragmentShader);
    textProgram = new shaderProgram_t(textVertexShader, textFragmentShader);

    debugInfo = new debug_info_t();
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
        snprintf(char_buf, bufsize, 
        "FPS: %.0f\n<%.2f,%.2f,%.2f>",
        fps, camera->position.x, camera->position.y, camera->position.z
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

    segment_t s_base(nullptr, z_axis, z_axis, 46.19, 7);
    segment_t s_6(&s_base, z_axis, z_axis, 35.98, 6);
    segment_t s_5(&s_6, y_axis, z_axis, 98.0, 5);
    segment_t s_4(&s_5, y_axis, z_axis, 96.0, 4);
    segment_t s_3(&s_4, y_axis, z_axis, 150.0, 3);

    s_base.load("xarm-sbase.stl");
    s_6.load("xarm-s6.stl");
    s_5.load("xarm-s5.stl");
    s_4.load("xarm-s4.stl");
    s_3.load("xarm-s3.stl");

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
        
        glBegin(GL_TRIANGLES);
            glColor3f(1.0f, 0.0f, 0.0f); // Red
            glVertex3f(-0.6f, -0.4f, 0.0f);
            glColor3f(0.0f, 1.0f, 0.0f); // Green
            glVertex3f(0.6f, -0.4f, 0.0f);
            glColor3f(0.0f, 0.0f, 1.0f); // Blue
            glVertex3f(0.0f, 0.6f, 0.0f);
        glEnd();

        s_3.render();
        s_4.render();
        s_5.render();
        s_6.render();
        s_base.render();
        
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
}
