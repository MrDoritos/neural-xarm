#include "shader.h"

bool shader_t::load(const std::string &path) {
    std::stringstream buffer;
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: Opening shader file: " << path << std::endl;
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
        std::cerr << "Error: Compiling shader: " << path << std::endl;
        std::cerr << infoLog << std::endl;
        glDeleteShader(shaderId);
        return glfail;
    }

    return glsuccess;
}