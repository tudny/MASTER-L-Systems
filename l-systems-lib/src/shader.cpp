#include <vector>
#include <fstream>
#include <stdexcept>
#include <string>
#include <map>
#include "shader.hpp"
#include "errors.hpp"
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

#define OPENGL_ERROR_BUFFER_SIZE 1024

static void read_source(const std::string &filename, std::vector<char> &buffer) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    buffer.resize(size + 1);
    file.read(buffer.data(), size);
    buffer[size] = '\0';

    if (!file) {
        throw std::runtime_error("Failed to read file: " + filename);
    }

    file.close();
}

unsigned int Shader::getShaderId() const {
    return shader_id;
}

Shader::Shader(const std::string &filename, GLenum type) {
    std::vector<char> shader_source;
    read_source(filename, shader_source);

    shader_id = glCreateShader(type);
    const char *source = shader_source.data();
    glShaderSource(shader_id, 1, &source, nullptr);
    glCompileShader(shader_id);

    int success;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info_log[OPENGL_ERROR_BUFFER_SIZE];
        glGetShaderInfoLog(shader_id, OPENGL_ERROR_BUFFER_SIZE, nullptr, info_log);
        throw std::runtime_error("Failed to compile shader: " + std::string(info_log));
    }

    GL_CHECK_ERROR();
}

Shader::~Shader() {
    glDeleteShader(shader_id);
    GL_CHECK_ERROR();
}

ShaderProgram::ShaderProgram(std::initializer_list<Shader> shaders) {
    program_id = glCreateProgram();
    for (const auto &shader: shaders) {
        glAttachShader(program_id, shader.getShaderId());
    }
    glLinkProgram(program_id);

    int success;
    glGetProgramiv(program_id, GL_LINK_STATUS, &success);
    if (!success) {
        char info_log[OPENGL_ERROR_BUFFER_SIZE];
        glGetProgramInfoLog(program_id, OPENGL_ERROR_BUFFER_SIZE, nullptr, info_log);
        throw std::runtime_error("Failed to link shader program: " + std::string(info_log));
    }

    GL_CHECK_ERROR();
}

ShaderProgram::~ShaderProgram() {
    glDeleteProgram(program_id);
    GL_CHECK_ERROR();
}

void ShaderProgram::use() const {
    glUseProgram(program_id);
    GL_CHECK_ERROR();
}

void ShaderProgram::unuse() const {
    glUseProgram(0);
    GL_CHECK_ERROR();
}

GLuint ShaderProgram::getProgramId() const {
    return program_id;
}

GLint ShaderProgram::get_uniform(const std::string &name) {
    return get_location(name, uniforms, glGetUniformLocation, "uniform");
}

GLint ShaderProgram::get_attribute(const std::string &name) {
    return get_location(name, attributes, glGetAttribLocation, "attribute");
}

GLint ShaderProgram::get_location(
        const std::string &name,
        std::map<std::string, GLint> &locations,
        const std::function<GLint(GLuint, const GLchar *)> &get_location_function,
        const std::string &resource_type
) const {
    auto it = locations.find(name);
    if (it == locations.end()) {
        GLint location = get_location_function(program_id, name.c_str());
        if (location == -1) {
            throw std::runtime_error("Failed to get " + resource_type + " location: " + name);
        }
        locations[name] = location;
        return location;
    } else {
        return it->second;
    }
}


void ShaderProgram::setUniform(
        const std::string &name,
        float x,
        float y,
        float z
) {
    glUniform3f(get_uniform(name), x, y, z);
}

void ShaderProgram::setUniform(const std::string &name, const glm::vec3 &v) {
    glUniform3fv(get_uniform(name), 1, value_ptr(v));
}

void ShaderProgram::setUniform(const std::string &name, const glm::dvec3 &v) {
    glUniform3dv(get_uniform(name), 1, value_ptr(v));
}

void ShaderProgram::setUniform(const std::string &name, const glm::vec4 &v) {
    glUniform4fv(get_uniform(name), 1, value_ptr(v));
}

void ShaderProgram::setUniform(const std::string &name, const glm::dvec4 &v) {
    glUniform4dv(get_uniform(name), 1, value_ptr(v));
}

void ShaderProgram::setUniform(const std::string &name, const glm::dmat4 &m) {
    glUniformMatrix4dv(get_uniform(name), 1, GL_FALSE, value_ptr(m));
}

void ShaderProgram::setUniform(const std::string &name, const glm::mat4 &m) {
    glUniformMatrix4fv(get_uniform(name), 1, GL_FALSE, value_ptr(m));
}

void ShaderProgram::setUniform(const std::string &name, const glm::mat3 &m) {
    glUniformMatrix3fv(get_uniform(name), 1, GL_FALSE, value_ptr(m));
}

void ShaderProgram::setUniform(const std::string &name, float val) {
    glUniform1f(get_uniform(name), val);
}

void ShaderProgram::setUniform(const std::string &name, int val) {
    glUniform1i(get_uniform(name), val);
}

void ShaderProgram::setUniform(const std::string &name, const std::function<void(GLint)> &callback) {
    callback(get_uniform(name));
}

void ShaderProgram::setAttribute(
        const std::string &name,
        GLint size,
        GLsizei stride,
        GLuint offset,
        GLboolean normalize,
        GLenum type
) {
    GLint loc = get_attribute(name);
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(
            loc, size, type, normalize, stride,
            reinterpret_cast<void *>(offset)
    );
    GL_CHECK_ERROR();
}

void ShaderProgram::setAttribute(
        const std::string &name,
        GLint size,
        GLsizei stride,
        GLuint offset,
        GLboolean normalize
) {
    setAttribute(name, size, stride, offset, normalize, GL_FLOAT);
}

void ShaderProgram::setAttribute(const std::string &name, GLint size, GLsizei stride, GLuint offset, GLenum type) {
    setAttribute(name, size, stride, offset, GL_FALSE, type);
}

void ShaderProgram::setAttribute(const std::string &name, GLint size, GLsizei stride, GLuint offset) {
    setAttribute(name, size, stride, offset, GL_FALSE, GL_FLOAT);
}
