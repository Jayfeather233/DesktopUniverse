#include "shader.hpp"
#include <glm/gtc/type_ptr.hpp>

namespace ogl {

Shader::Shader(shader_type_t type, const std::string &content) : is_compiled(false)
{
    auto c = content.c_str();
    id = glCreateShader(static_cast<GLenum>(type));
    glShaderSource(id, 1, &c, NULL);
    source = content;
    compile();
}

Shader::Shader(shader_type_t type, const fs::path &path)
{
    shader_path = path;
    if (!fs::exists(path)) {
        throw std::runtime_error(fmt::format("Shader path {} not exist", path.string()));
    }
    std::stringstream contentstream;
    std::string line;
    {
        std::ifstream f{path};
        while (std::getline(f, line)) {
            if (line.find("#include") == 0) {
                // TODO simple preprocessor
            }
            contentstream << line << std::endl;
        }
    }
    // f.close();
    auto content = contentstream.str();
    auto c = content.c_str();
    id = glCreateShader(static_cast<GLenum>(type));
    glShaderSource(id, 1, &c, NULL);
    source = std::move(content);
    compile();
}

Shader::~Shader() { glDeleteShader(id); }

void Shader::compile()
{
    glCompileShader(id);
    GLint success;
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        GLchar infoLog[1024];
        glGetShaderInfoLog(id, 1024, NULL, infoLog);
        // fmt::print(stderr, "Shader path: {}\n",
        //            static_cast<std::string>(this->shader_path));
        throw std::runtime_error(
            fmt::format("Shader Error:\nShader Path: {}\nError Info: {}\n", shader_path.string(), infoLog));
    }
    is_compiled = true;
}

Program::Program()
{
    id = glCreateProgram();
    is_linked = false;
}

Program::~Program() { glDeleteProgram(id); }

void Program::attach(const std::unique_ptr<Shader> &shader)
{
    if (!shader->is_compiled)
        shader->compile();
    glAttachShader(id, shader->id);
}

void Program::link()
{
    glLinkProgram(id);
    GLint success;
    glGetProgramiv(id, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        GLchar infoLog[1024];
        glGetProgramInfoLog(id, 1024, NULL, infoLog);
        throw std::runtime_error(infoLog);
    }
    else {
        is_linked = true;
    }
}

void Program::bind()
{
    if (!is_linked)
        link();
    if (!is_linked)
        return;
    glUseProgram(id);
}

void Program::release() const { glUseProgram(0); }

void Program::setUniform(const std::string &name, int v)
{
    GLuint loc = glGetUniformLocation(id, name.c_str());
    glUniform1i(loc, v);
}

void Program::setUniform(const std::string &name, float v)
{
    GLuint loc = glGetUniformLocation(id, name.c_str());
    glUniform1f(loc, v);
}

void Program::setUniform(const std::string &name, const Eigen::Vector2f &v)
{
    GLuint loc = glGetUniformLocation(id, name.c_str());
    glUniform2f(loc, v[0], v[1]);
}

void Program::setUniform(const std::string &name, const Eigen::Vector3f &v)
{
    GLuint loc = glGetUniformLocation(id, name.c_str());
    glUniform3f(loc, v[0], v[1], v[2]);
}

void Program::setUniform(const std::string &name, const Eigen::Vector4f &v)
{
    GLuint loc = glGetUniformLocation(id, name.c_str());
    glUniform4f(loc, v[0], v[1], v[2], v[3]);
}

void Program::setUniform(const std::string &name, const Eigen::Matrix3f &v)
{
    GLuint loc = glGetUniformLocation(id, name.c_str());
    glUniformMatrix3fv(loc, 1, GL_FALSE, v.data());
}

void Program::setUniform(const std::string &name, const Eigen::Matrix4f &v)
{
    GLuint loc = glGetUniformLocation(id, name.c_str());
    glUniformMatrix4fv(loc, 1, GL_FALSE, v.data());
}

void Program::setUniform(const std::string &name, const glm::vec2 &v)
{
    GLuint loc = glGetUniformLocation(id, name.c_str());
    glUniform2fv(loc, 1, glm::value_ptr(v));
}
void Program::setUniform(const std::string &name, const glm::vec3 &v)
{
    GLuint loc = glGetUniformLocation(id, name.c_str());
    glUniform3fv(loc, 1, glm::value_ptr(v));
}
void Program::setUniform(const std::string &name, const glm::vec4 &v)
{
    GLuint loc = glGetUniformLocation(id, name.c_str());
    glUniform4fv(loc, 1, glm::value_ptr(v));
}
void Program::setUniform(const std::string &name, const glm::dvec4 &v)
{
    GLuint loc = glGetUniformLocation(id, name.c_str());
    glUniform4dv(loc, 1, glm::value_ptr(v));
}
void Program::setUniform(const std::string &name, const glm::mat3 &v)
{
    GLuint loc = glGetUniformLocation(id, name.c_str());
    glUniformMatrix3fv(loc, 1, GL_FALSE, glm::value_ptr(v));
}
void Program::setUniform(const std::string &name, const glm::mat4 &v)
{
    GLuint loc = glGetUniformLocation(id, name.c_str());
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(v));
}
void Program::setUniform(const std::string &name, const glm::f64mat4 &v)
{
    GLuint loc = glGetUniformLocation(id, name.c_str()); //TODO Optimize GetLoc, buffer it
    glUniformMatrix4dv(loc, 1, GL_FALSE, glm::value_ptr(v));
}

} // namespace ogl
