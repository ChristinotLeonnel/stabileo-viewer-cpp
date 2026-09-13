// =============================================================================
//  Shader.cpp — Compilation et liaison de programmes shaders OpenGL
// =============================================================================

#include "core/Shader.h"
#include <cstdio>
#include <vector>

Shader::~Shader() { cleanup(); }

void Shader::cleanup() {
    if (id) { glDeleteProgram(id); id = 0; }
}

GLuint Shader::compileStage(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);

    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(static_cast<size_t>(len));
        glGetShaderInfoLog(s, len, nullptr, log.data());
        const char* label = (type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";
        fprintf(stderr, "[Shader] %s compile error:\n%s\n", label, log.data());
        glDeleteShader(s);
        return 0;
    }
    return s;
}

bool Shader::compile(const char* vertSrc, const char* fragSrc) {
    cleanup();

    GLuint vs = compileStage(GL_VERTEX_SHADER, vertSrc);
    if (!vs) return false;

    GLuint fs = compileStage(GL_FRAGMENT_SHADER, fragSrc);
    if (!fs) { glDeleteShader(vs); return false; }

    id = glCreateProgram();
    glAttachShader(id, vs);
    glAttachShader(id, fs);
    glLinkProgram(id);

    // Shaders sont détachables après liaison
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = 0;
    glGetProgramiv(id, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(id, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(static_cast<size_t>(len));
        glGetProgramInfoLog(id, len, nullptr, log.data());
        fprintf(stderr, "[Shader] Link error:\n%s\n", log.data());
        cleanup();
        return false;
    }
    return true;
}
