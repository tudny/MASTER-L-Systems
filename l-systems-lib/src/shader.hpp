
#ifndef LSYSTEMS_LIB_SHADER_HPP
#define LSYSTEMS_LIB_SHADER_HPP

#include <string>
#include <map>
#include <functional>
#include "GL/glew.h"
#include <glm/glm.hpp>

class Shader;

class ShaderProgram;

class Shader {
public:
    Shader(const std::string &filename, GLenum type);

    ~Shader();

    [[nodiscard]] GLuint getShaderId() const;

private:
    GLuint shader_id;
};

class ShaderProgram {
public:
    ShaderProgram(std::initializer_list<Shader> shaders);

    ~ShaderProgram();

    void use() const;

    void unuse() const;

    [[nodiscard]] GLuint getProgramId() const;

    [[nodiscard]] GLint get_uniform(const std::string &name);

    [[nodiscard]] GLint get_attribute(const std::string &name);

    void setUniform(const std::string &name, float x, float y, float z);

    void setUniform(const std::string &name, const glm::vec3 &v);

    void setUniform(const std::string &name, const glm::dvec3 &v);

    void setUniform(const std::string &name, const glm::vec4 &v);

    void setUniform(const std::string &name, const glm::dvec4 &v);

    void setUniform(const std::string &name, const glm::dmat4 &m);

    void setUniform(const std::string &name, const glm::mat4 &m);

    void setUniform(const std::string &name, const glm::mat3 &m);

    void setUniform(const std::string &name, float val);

    void setUniform(const std::string &name, int val);

    void setUniform(const std::string &name, const std::function<void(GLint)> &callback);

    void setAttribute(
            const std::string &name,
            GLint size,
            GLsizei stride,
            GLuint offset,
            GLboolean normalize,
            GLenum type
    );

    void setAttribute(const std::string &name, GLint size, GLsizei stride, GLuint offset, GLboolean normalize);

    void setAttribute(const std::string &name, GLint size, GLsizei stride, GLuint offset, GLenum type);

    void setAttribute(const std::string &name, GLint size, GLsizei stride, GLuint offset);

private:

    GLint get_location(
            const std::string &name,
            std::map<std::string, GLint> &locations,
            const std::function<GLint(GLuint, const GLchar *)> &get_location_function,
            const std::string &resource_type
    ) const;

    GLuint program_id;

    std::map<std::string, GLint> uniforms;
    std::map<std::string, GLint> attributes;
};

#endif // LSYSTEMS_LIB_SHADER_HPP
