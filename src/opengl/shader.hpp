#pragma once

#include <GL/glew.h>
#include <fmt/core.h>

#include <array>
#include <eigen3/Eigen/Core>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace fs = std::filesystem;

namespace ogl {
enum class shader_type_t { VERT = GL_VERTEX_SHADER, FRAG = GL_FRAGMENT_SHADER, NONE = 0 };

class Shader {
public:
    // from content
    Shader(shader_type_t, const std::string &);
    // from file
    Shader(shader_type_t, const fs::path &);

    Shader(const Shader &) = delete;
    Shader &operator=(const Shader &) = delete;
    Shader(Shader &&) = delete;
    Shader &operator=(Shader &&) = delete;

    ~Shader();

    void compile();
    // TODO preprocess some marco like #include
    // void preprocess();
    std::string source;
    fs::path shader_path;
    GLuint id;
    bool is_compiled;
};

class Program {
public:
    Program();

    Program(const Program &) = delete;
    Program &operator=(const Program &) = delete;

    Program(Program &&) = default;
    Program &operator=(Program &&) = default;

    ~Program();

    void attach(const std::unique_ptr<Shader> &);
    void link();

    void bind();
    void release() const;

    void setUniform(const std::string &, int);
    void setUniform(const std::string &, float);
    void setUniform(const std::string &, const Eigen::Vector2f &);
    void setUniform(const std::string &, const Eigen::Vector3f &);
    void setUniform(const std::string &, const Eigen::Vector4f &);
    void setUniform(const std::string &, const Eigen::Matrix3f &);
    void setUniform(const std::string &, const Eigen::Matrix4f &);
    void setUniform(const std::string &, const glm::vec2 &);
    void setUniform(const std::string &, const glm::vec3 &);
    void setUniform(const std::string &, const glm::vec4 &);
    void setUniform(const std::string &, const glm::dvec4 &);
    void setUniform(const std::string &, const glm::mat3 &);
    void setUniform(const std::string &, const glm::mat4 &);
    void setUniform(const std::string &, const glm::f64mat4 &);

private:
    GLenum id;
    bool is_linked = false;
    // std::set<std::unique_ptr<Shader>> shaders;
};

static inline std::shared_ptr<Program> programFromFiles(const fs::path &shaderDir,
                                                        const std::string &vertShaderFilename,
                                                        const std::string &fragShaderFilename)
{
    auto program = std::make_shared<Program>();

    program->attach(std::make_unique<Shader>(shader_type_t::VERT, shaderDir / vertShaderFilename));
    program->attach(std::make_unique<Shader>(shader_type_t::FRAG, shaderDir / fragShaderFilename));

    program->link();
    return program;
}
} // namespace ogl
