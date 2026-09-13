#pragma once
// =============================================================================
//  Shader.h — Compilateur et gestionnaire de shaders OpenGL
// =============================================================================

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

class Shader {
public:
    GLuint id = 0;

    Shader() = default;
    ~Shader();

    // Non-copiable
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    // Déplaçable
    Shader(Shader&& o) noexcept : id(o.id) { o.id = 0; }
    Shader& operator=(Shader&& o) noexcept {
        if (this != &o) { cleanup(); id = o.id; o.id = 0; }
        return *this;
    }

    /// Compile et lie un programme vertex + fragment.
    /// Retourne true si succès, false et affiche les erreurs sinon.
    bool compile(const char* vertSrc, const char* fragSrc);

    /// Active ce programme shader.
    void use() const { glUseProgram(id); }

    // ---- Setters d'uniformes ----
    void setMat4 (const char* n, const glm::mat4& m) const { glUniformMatrix4fv(loc(n), 1, GL_FALSE, glm::value_ptr(m)); }
    void setMat3 (const char* n, const glm::mat3& m) const { glUniformMatrix3fv(loc(n), 1, GL_FALSE, glm::value_ptr(m)); }
    void setVec3 (const char* n, const glm::vec3& v) const { glUniform3fv(loc(n), 1, glm::value_ptr(v)); }
    void setFloat(const char* n, float f)            const { glUniform1f(loc(n), f); }
    void setInt  (const char* n, int   i)            const { glUniform1i(loc(n), i); }

private:
    GLint loc(const char* n) const { return glGetUniformLocation(id, n); }
    GLuint compileStage(GLenum type, const char* src);
    void cleanup();
};
