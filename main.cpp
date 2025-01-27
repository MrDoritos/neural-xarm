#include <iostream>
#include <limits>

#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <GL/gl.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "stl_reader.h"

struct camera_t;
struct mesh_t;
struct shader_t;

int width = 1600, height = 900;
float lastTime;
float mouseSensitivity = 0.05f;
float preciseSpeed = 0.1f;
float movementSpeed = 10.0f;
float rapidSpeed = 100.0f;
GLFWwindow* window;
GLuint program;
GLint uni_projection, uni_model, uni_norm, uni_view;

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

struct segment_t : public mesh_t {
    segment_t(segment_t *parent, glm::vec3 rotation_axis, glm::vec3 initial_direction, float length, int servo_num) {
        this->parent = parent;
        this->rotation_axis = rotation_axis;
        this->initial_direction = initial_direction;
        this->length = length;
        this->servo_num = servo_num;
        this->rotation = 12.0f;
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
            //return glm::rotate(base_rotation, glm::radians(90.0f), glm::vec3(x_axis));
            return base_rotation;
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
        matrix = glm::rotate(matrix, glm::radians(-90.0f), glm::vec3(x_axis));
        //matrix = glm::rotate(matrix, glm::radians(rotation), glm::vec3(rotation_axis));
        matrix *= get_rotation_matrix();
        matrix = glm::translate(matrix, origin);
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
        renderVector(origin, mat[0]);
        renderVector(origin, mat[1]);
        renderVector(origin, mat[2]);
    }

    void renderDebug() {
        glm::vec3 origin = get_origin();
        renderVector(origin, get_origin());
        renderVector(origin, get_segment_vector());
        renderMatrix(origin, get_rotation_matrix());
    }

    void render() {
        glUseProgram(program);
        glm::mat4 model = get_model_transform();
        glUniformMatrix4fv(uni_model, 1, GL_FALSE, glm::value_ptr(model));
        renderDebug();
        mesh_t::render();
    }

    float model_scale = 0.1;
    glm::vec3 direction, rotation_axis, initial_direction;
    segment_t *parent;
    float length, rotation;
    int servo_num;
};

struct shader_t {
    GLuint shaderId;

    void load(const char *filename, GLenum shaderType) {
        std::stringstream buffer;
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error opening shader file: " << filename << std::endl;
            return;
        }
        buffer << file.rdbuf();
        std::string shaderCodeStr = buffer.str();
        const char* shaderCode = shaderCodeStr.c_str();
        int length = shaderCodeStr.size();
        shaderId = glCreateShader(shaderType);
        glShaderSource(shaderId, 1, &shaderCode, 0);
        glCompileShader(shaderId);
        GLint compiled = 0;
        glGetShaderiv(shaderId, GL_COMPILE_STATUS, &compiled);
        GLint infoLen = 512;
        if (compiled == GL_FALSE) {
            char infoLog[infoLen];
            glGetShaderInfoLog(shaderId, infoLen, nullptr, infoLog);
            std::cerr << "Error compiling shader: " << filename << std::endl;
            std::cerr << infoLog << std::endl;
            std::cerr << shaderCodeStr << std::endl;
            glDeleteShader(shaderId);
        }
    }
};

camera_t camera;
shader_t vertex_shader, fragment_shader;

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void cursor_position_callback(GLFWwindow *window, double x, double y);
void handle_keyboard(GLFWwindow* window, float deltaTime);

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


    program = glCreateProgram();
    vertex_shader.load("vertex.glsl", GL_VERTEX_SHADER);
    fragment_shader.load("fragment.glsl", GL_FRAGMENT_SHADER);
    glAttachShader(program, vertex_shader.shaderId);
    glAttachShader(program, fragment_shader.shaderId);
    glLinkProgram(program);

    int success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        char infoLog[513];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cout << "Error linking shader\n" << infoLog << std::endl;
    }

    glDeleteShader(vertex_shader.shaderId);
    glDeleteShader(fragment_shader.shaderId);

    uni_model = glGetUniformLocation(program, "model");
    uni_view = glGetUniformLocation(program, "view");
    uni_projection = glGetUniformLocation(program, "projection");
    uni_norm = glGetUniformLocation(program, "norm");

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
		//glEnable(GL_ALPHA_TEST);
		//glEnable(GL_BLEND);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glm::mat4 model = glm::scale(glm::mat4(1.0), glm::vec3(0.1));
        //glm::mat4 model = glm::translate(glm::mat4(1.0), glm::vec3(0));
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(camera.fov),
                                                (float) width / height, camera.near, camera.far);
        glm::mat3 norm(model);
        norm = glm::inverse(norm);
        norm = glm::transpose(norm);

        glUseProgram(program);

        glUniformMatrix4fv(uni_model, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(uni_view, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(uni_projection, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(uni_norm, 1, glm::value_ptr(norm));
        
        s_3.render();
        s_4.render();
        s_5.render();

        //glUseProgram(0);

        glBegin(GL_TRIANGLES);
            glColor3f(1.0f, 0.0f, 0.0f); // Red
            glVertex3f(-0.6f, -0.4f, 0.0f);
            glColor3f(0.0f, 1.0f, 0.0f); // Green
            glVertex3f(0.6f, -0.4f, 0.0f);
            glColor3f(0.0f, 0.0f, 1.0f); // Blue
            glVertex3f(0.0f, 0.6f, 0.0f);
        glEnd();
        

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
    camera.mouseMove(window, x, y);
}

void handle_keyboard(GLFWwindow* window, float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    camera.keyboard(window, deltaTime);
}
