
#ifndef LSYSTEMS_LIB_SHADER_HPP
#define LSYSTEMS_LIB_SHADER_HPP

#include <string>
#include <map>
#include <functional>
#include "GL/glew.h"
#include <glm/glm.hpp>

class Shader;

class ShaderProgram;

/// Class representing a shader
class Shader {
public:

    /**
     * @brief Shader constructor
     *
     * Shader constructor creates a shader object with given filename and type.
     *
     * @param filename Filename
     * @param type Type
     */
    Shader(const std::string &filename, GLenum type);

    /**
     * @brief Shader destructor
     *
     * Shader destructor is a default destructor.
     */
    ~Shader();

    /**
     * @brief Get shader id
     *
     * Get the shader id.
     *
     * @return Shader id
     */
    [[nodiscard]] GLuint getShaderId() const;

private:
    /// Shader handle
    GLuint shader_id;
};

/// Class representing a shader program
class ShaderProgram {
public:
    /**
     * @brief ShaderProgram constructor
     *
     * ShaderProgram constructor creates a shader program object with given shaders.
     *
     * @param shaders Shaders
     */
    ShaderProgram(std::initializer_list<Shader> shaders);

    /**
     * @brief ShaderProgram destructor
     *
     * ShaderProgram destructor is a default destructor.
     */
    ~ShaderProgram();

    /**
     * @brief Use the shader program
     *
     * Use the shader program.
     */
    void use() const;

    /**
     * @brief Unuse the shader program
     *
     * Unuse the shader program.
     */
    void unuse() const;

    /**
     * @brief Get program id
     *
     * Get the program id.
     *
     * @return Program id
     */
    [[nodiscard]] GLuint getProgramId() const;

    /**
     * @brief Get uniform location
     *
     * Get the uniform location.
     *
     * @param name Name
     * @return Uniform location
     */
    [[nodiscard]] GLint get_uniform(const std::string &name);

    /**
     * @brief Get attribute location
     *
     * Get the attribute location.
     *
     * @param name Name
     * @return Attribute location
     */
    [[nodiscard]] GLint get_attribute(const std::string &name);

    /**
     * @brief Set uniform
     *
     * Set the uniform with given name and value.
     *
     * @param name Name
     * @param x X
     * @param y Y
     * @param z Z
     */
    void setUniform(const std::string &name, float x, float y, float z);

    /**
     * @brief Set uniform
     *
     * Set the uniform with given name and value.
     *
     * @param name Name
     * @param v Value
     */
    void setUniform(const std::string &name, const glm::vec3 &v);

    /**
     * @brief Set uniform
     *
     * Set the uniform with given name and value.
     *
     * @param name Name
     * @param v Value
     */
    void setUniform(const std::string &name, const glm::dvec3 &v);

    /**
     * @brief Set uniform
     *
     * Set the uniform with given name and value.
     *
     * @param name Name
     * @param v Value
     */
    void setUniform(const std::string &name, const glm::vec4 &v);

    /**
     * @brief Set uniform
     *
     * Set the uniform with given name and value.
     *
     * @param name Name
     * @param v Value
     */
    void setUniform(const std::string &name, const glm::dvec4 &v);

    /**
     * @brief Set uniform
     *
     * Set the uniform with given name and value.
     *
     * @param name Name
     * @param m Value
     */
    void setUniform(const std::string &name, const glm::dmat4 &m);

    /**
     * @brief Set uniform
     *
     * Set the uniform with given name and value.
     *
     * @param name Name
     * @param m Value
     */
    void setUniform(const std::string &name, const glm::mat4 &m);

    /**
     * @brief Set uniform
     *
     * Set the uniform with given name and value.
     *
     * @param name Name
     * @param m Value
     */
    void setUniform(const std::string &name, const glm::mat3 &m);

    /**
     * @brief Set uniform
     *
     * Set the uniform with given name and value.
     *
     * @param name Name
     * @param val Value
     */
    void setUniform(const std::string &name, float val);

    /**
     * @brief Set uniform
     *
     * Set the uniform with given name and value.
     *
     * @param name Name
     * @param val Value
     */
    void setUniform(const std::string &name, int val);

    void setUniform(const std::string &name, uint32_t val);

    /**
     * @brief Set uniform
     *
     * Set the uniform with given name and value.
     *
     * @param name Name
     * @param callback Callback
     */
    void setUniform(const std::string &name, const std::function<void(GLint)> &callback);

    /**
     * @brief Set attribute
     *
     * Set the attribute with given name, size, stride, offset, normalize and type.
     *
     * @param name Name
     * @param size Size
     * @param stride Stride
     * @param offset Offset
     * @param normalize Normalize
     * @param type Type
     */
    void setAttribute(
            const std::string &name,
            GLint size,
            GLsizei stride,
            GLuint offset,
            GLboolean normalize,
            GLenum type
    );

    /**
     * @brief Set attribute
     *
     * Set the attribute with given name, size, stride, offset and normalize.
     *
     * @param name Name
     * @param size Size
     * @param stride Stride
     * @param offset Offset
     * @param normalize Normalize
     */
    void setAttribute(const std::string &name, GLint size, GLsizei stride, GLuint offset, GLboolean normalize);

    /**
     * @brief Set attribute
     *
     * Set the attribute with given name, size, stride, offset and type.
     *
     * @param name Name
     * @param size Size
     * @param stride Stride
     * @param offset Offset
     * @param type Type
     */
    void setAttribute(const std::string &name, GLint size, GLsizei stride, GLuint offset, GLenum type);

    /**
     * @brief Set attribute
     *
     * Set the attribute with given name, size, stride and offset.
     *
     * @param name Name
     * @param size Size
     * @param stride Stride
     * @param offset Offset
     */
    void setAttribute(const std::string &name, GLint size, GLsizei stride, GLuint offset);

private:

    /**
     * @brief Get location
     *
     * Get the location with given name, locations, get_location_function and resource_type.
     *
     * @param name Name
     * @param locations Locations
     * @param get_location_function Get location function
     * @param resource_type Resource type
     * @return Location
     */
    GLint get_location(
            const std::string &name,
            std::map<std::string, GLint> &locations,
            const std::function<GLint(GLuint, const GLchar *)> &get_location_function,
            const std::string &resource_type
    ) const;

    /// Program handle
    GLuint program_id;

    /// Uniforms map used for caching
    std::map<std::string, GLint> uniforms;
    /// Attributes map used for caching
    std::map<std::string, GLint> attributes;
};

#endif // LSYSTEMS_LIB_SHADER_HPP
