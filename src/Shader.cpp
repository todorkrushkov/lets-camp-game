
#include "Shader.h"

#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

static std::string readFile(const std::string& path) {
    std::ifstream file(path);

    if (!file.is_open()) {
        throw std::runtime_error("Cannot open shader: " + path);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

static unsigned int compileShader(GLenum type, const std::string& source) {
    unsigned int shader = glCreateShader(type);

    const char* code = source.c_str();
    glShaderSource(shader, 1, &code, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        char log[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, log);
        std::cerr << "Shader compilation error:\n" << log << '\n';

        glDeleteShader(shader);
        throw std::runtime_error("Shader compilation failed");
    }

    return shader;
}

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vertexCode = readFile(vertexPath);
    std::string fragmentCode = readFile(fragmentPath);

    unsigned int vertex = compileShader(GL_VERTEX_SHADER, vertexCode);

    unsigned int fragment;

    try {
        fragment = compileShader(GL_FRAGMENT_SHADER, fragmentCode);
    } catch (...) {
        glDeleteShader(vertex);
        throw;
    }

    ID = glCreateProgram();

    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);

    int success;
    glGetProgramiv(ID, GL_LINK_STATUS, &success);

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    if (!success) {
        char log[1024];
        glGetProgramInfoLog(ID, 1024, nullptr, log);
        std::cerr << "Shader linking error:\n" << log << '\n';

        glDeleteProgram(ID);
        throw std::runtime_error("Shader linking failed");
    }
}

Shader::~Shader() {
    if (ID)
        glDeleteProgram(ID);
}

void Shader::use() const {
    glUseProgram(ID);
}

void Shader::setMat4(const std::string& name, const glm::mat4& matrix) const {
    int location = glGetUniformLocation(ID, name.c_str());

    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
}
