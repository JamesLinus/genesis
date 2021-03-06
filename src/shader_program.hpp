#ifndef SHADER_PROGRAM_HPP
#define SHADER_PROGRAM_HPP

#include "util.hpp"
#include "glm.hpp"
#include "glfw.hpp"

class ShaderProgram {
public:
    ShaderProgram(
            const char *vertex_shader_source,
            const char *fragment_shader_source,
            const char *geometry_shader_source);
    ~ShaderProgram();

    void bind() const {
        glUseProgram(program_id);
    }

    GLint attrib_location(const char *name) {
        GLint id = glGetAttribLocation(program_id, name);
        if (id == -1)
            panic("invalid attrib: %s", name);
        return id;
    }

    GLint uniform_location(const char *name) {
        GLint id = glGetUniformLocation(program_id, name);
        if (id == -1)
            panic("invalid uniform: %s", name);
        return id;
    }

    void set_uniform(GLint uniformId, int value) const;
    void set_uniform(GLint uniformId, float value) const;
    void set_uniform(GLint uniformId, const glm::vec3 &value) const;
    void set_uniform(GLint uniformId, const glm::vec4 &value) const;
    void set_uniform(GLint uniformId, const glm::mat4 &value) const;
    void set_uniform(GLint uniformId, const glm::mat3 &value) const;

private:

    GLuint program_id;
    GLuint vertex_id;
    GLuint fragment_id;

    GLuint geometry_id;
    bool have_geometry;
};

#endif
