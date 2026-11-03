#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <string>

class Shader {
  public:
    unsigned int ID = 0;

    Shader(const std::string& vertexPath, const std::string& fragmentPath);

    ~Shader();
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    void use() const;

    void setMat4(const std::string& name, const glm::mat4& matrix) const;
};
