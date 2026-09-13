#pragma once
// =============================================================================
//  Mesh.h — Wrapper VAO / VBO / EBO OpenGL + générateurs de primitives
// =============================================================================

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

// ---- Types de vertex ----

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
};

struct ColorVertex {
    glm::vec3 position;
    glm::vec3 color;
};

struct HeatmapVertex {
    glm::vec3 position;
    glm::vec3 normal;
    float     value;   // 0..1
};

// ---- Classe Mesh ----

class Mesh {
public:
    GLuint   vao = 0, vbo = 0, ebo = 0;
    GLsizei  indexCount  = 0;
    GLsizei  vertexCount = 0;
    GLenum   drawMode    = GL_TRIANGLES;

    Mesh() = default;
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    Mesh(Mesh&& o) noexcept;
    Mesh& operator=(Mesh&& o) noexcept;

    // ---- Upload de données ----
    void upload         (const std::vector<Vertex>&        verts, const std::vector<uint32_t>& idx);
    void uploadColor    (const std::vector<ColorVertex>&   verts, const std::vector<uint32_t>& idx);
    void uploadHeatmap  (const std::vector<HeatmapVertex>& verts, const std::vector<uint32_t>& idx);
    void uploadLines    (const std::vector<ColorVertex>&   verts);
    void uploadLineStrip(const std::vector<ColorVertex>&   verts);

    // ---- Rendu ----
    void draw() const;

    // ---- Générateurs de primitives ----
    static Mesh createSphere  (float radius, int sectors = 24, int stacks = 16);
    static Mesh createCylinder(float radius, float height, int sectors = 16);
    static Mesh createCone    (float radius, float height, int sectors = 16);
    static Mesh createArrow   (float shaftR, float shaftL, float headR, float headL, int sectors = 12);
    static Mesh createBox     (float w, float h, float d);

    /// Crée un mesh à partir d'un contour 2D extrudé entre deux tranches 3D.
    /// contour : points 2D (Y, Z) de la section dans le plan local
    /// normals2D : normales 2D correspondantes
    /// Les points sont placés sur les tranches aux positions/orientations données.
    static Mesh createExtruded(
        const std::vector<glm::vec2>& contour,
        const std::vector<glm::vec2>& normals2D,
        const std::vector<glm::vec3>& axisPoints,
        const std::vector<glm::vec3>& axisUp,
        const std::vector<glm::vec3>& axisFwd);

private:
    void cleanup();
};
