// =============================================================================
//  Mesh.cpp — VAO/VBO/EBO + générateurs de primitives 3D
// =============================================================================

#include "core/Mesh.h"
#include <glm/gtc/constants.hpp>
#include <cmath>
#include <algorithm>

// ---- Move semantics ----
Mesh::~Mesh() { cleanup(); }

Mesh::Mesh(Mesh&& o) noexcept
    : vao(o.vao), vbo(o.vbo), ebo(o.ebo),
      indexCount(o.indexCount), vertexCount(o.vertexCount), drawMode(o.drawMode)
{
    o.vao = o.vbo = o.ebo = 0;
    o.indexCount = o.vertexCount = 0;
}

Mesh& Mesh::operator=(Mesh&& o) noexcept {
    if (this != &o) {
        cleanup();
        vao = o.vao; vbo = o.vbo; ebo = o.ebo;
        indexCount = o.indexCount; vertexCount = o.vertexCount; drawMode = o.drawMode;
        o.vao = o.vbo = o.ebo = 0;
        o.indexCount = o.vertexCount = 0;
    }
    return *this;
}

void Mesh::cleanup() {
    if (ebo) glDeleteBuffers(1, &ebo);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (vao) glDeleteVertexArrays(1, &vao);
    vao = vbo = ebo = 0;
    indexCount = vertexCount = 0;
}

// ---- Upload : Vertex (pos + normal) ----
void Mesh::upload(const std::vector<Vertex>& verts, const std::vector<uint32_t>& idx) {
    cleanup();
    drawMode = GL_TRIANGLES;
    indexCount = static_cast<GLsizei>(idx.size());
    vertexCount = static_cast<GLsizei>(verts.size());

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(verts.size() * sizeof(Vertex)),
                 verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(idx.size() * sizeof(uint32_t)),
                 idx.data(), GL_STATIC_DRAW);

    // location 0 : position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, position)));
    // location 1 : normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, normal)));

    glBindVertexArray(0);
}

// ---- Upload : ColorVertex (pos + color) ----
void Mesh::uploadColor(const std::vector<ColorVertex>& verts, const std::vector<uint32_t>& idx) {
    cleanup();
    drawMode = GL_TRIANGLES;
    indexCount = static_cast<GLsizei>(idx.size());
    vertexCount = static_cast<GLsizei>(verts.size());

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(verts.size() * sizeof(ColorVertex)),
                 verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(idx.size() * sizeof(uint32_t)),
                 idx.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ColorVertex),
                          reinterpret_cast<void*>(offsetof(ColorVertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ColorVertex),
                          reinterpret_cast<void*>(offsetof(ColorVertex, color)));
    glBindVertexArray(0);
}

// ---- Upload : HeatmapVertex (pos + normal + value) ----
void Mesh::uploadHeatmap(const std::vector<HeatmapVertex>& verts, const std::vector<uint32_t>& idx) {
    cleanup();
    drawMode = GL_TRIANGLES;
    indexCount = static_cast<GLsizei>(idx.size());
    vertexCount = static_cast<GLsizei>(verts.size());

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(verts.size() * sizeof(HeatmapVertex)),
                 verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(idx.size() * sizeof(uint32_t)),
                 idx.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(HeatmapVertex),
                          reinterpret_cast<void*>(offsetof(HeatmapVertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(HeatmapVertex),
                          reinterpret_cast<void*>(offsetof(HeatmapVertex, normal)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(HeatmapVertex),
                          reinterpret_cast<void*>(offsetof(HeatmapVertex, value)));
    glBindVertexArray(0);
}

// ---- Upload : Lines (ColorVertex, GL_LINES) ----
void Mesh::uploadLines(const std::vector<ColorVertex>& verts) {
    cleanup();
    drawMode = GL_LINES;
    vertexCount = static_cast<GLsizei>(verts.size());

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(verts.size() * sizeof(ColorVertex)),
                 verts.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ColorVertex),
                          reinterpret_cast<void*>(offsetof(ColorVertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ColorVertex),
                          reinterpret_cast<void*>(offsetof(ColorVertex, color)));
    glBindVertexArray(0);
}

// ---- Upload : Line strip ----
void Mesh::uploadLineStrip(const std::vector<ColorVertex>& verts) {
    uploadLines(verts);
    drawMode = GL_LINE_STRIP;
}

// ---- Draw ----
void Mesh::draw() const {
    if (!vao) return;
    glBindVertexArray(vao);
    if (indexCount > 0)
        glDrawElements(drawMode, indexCount, GL_UNSIGNED_INT, nullptr);
    else if (vertexCount > 0)
        glDrawArrays(drawMode, 0, vertexCount);
    glBindVertexArray(0);
}

// ==========================================================================
//  Primitives
// ==========================================================================

static constexpr float PI = glm::pi<float>();
static constexpr float TWO_PI = 2.0f * PI;

// ---- Sphère UV ----
Mesh Mesh::createSphere(float radius, int sectors, int stacks) {
    std::vector<Vertex> verts;
    std::vector<uint32_t> idx;

    for (int i = 0; i <= stacks; ++i) {
        float phi = PI * static_cast<float>(i) / static_cast<float>(stacks);
        float sinP = std::sin(phi), cosP = std::cos(phi);
        for (int j = 0; j <= sectors; ++j) {
            float theta = TWO_PI * static_cast<float>(j) / static_cast<float>(sectors);
            float sinT = std::sin(theta), cosT = std::cos(theta);
            glm::vec3 n(cosT * sinP, cosP, sinT * sinP);
            verts.push_back({n * radius, n});
        }
    }

    for (int i = 0; i < stacks; ++i) {
        int row  = i * (sectors + 1);
        int nrow = (i + 1) * (sectors + 1);
        for (int j = 0; j < sectors; ++j) {
            uint32_t a = static_cast<uint32_t>(row + j);
            uint32_t b = static_cast<uint32_t>(nrow + j);
            uint32_t c = static_cast<uint32_t>(nrow + j + 1);
            uint32_t d = static_cast<uint32_t>(row + j + 1);
            idx.insert(idx.end(), {a, b, c, a, c, d});
        }
    }

    Mesh m;
    m.upload(verts, idx);
    return m;
}

// ---- Cylindre (axe Y, base à y=0, sommet à y=height) ----
Mesh Mesh::createCylinder(float radius, float height, int sectors) {
    std::vector<Vertex> verts;
    std::vector<uint32_t> idx;

    for (int i = 0; i <= 1; ++i) {
        float y = static_cast<float>(i) * height;
        for (int j = 0; j <= sectors; ++j) {
            float theta = TWO_PI * static_cast<float>(j) / static_cast<float>(sectors);
            float cosT = std::cos(theta), sinT = std::sin(theta);
            glm::vec3 n(cosT, 0.0f, sinT);
            verts.push_back({{cosT * radius, y, sinT * radius}, n});
        }
    }

    int s1 = sectors + 1;
    for (int j = 0; j < sectors; ++j) {
        uint32_t a = static_cast<uint32_t>(j);
        uint32_t b = static_cast<uint32_t>(j + s1);
        uint32_t c = static_cast<uint32_t>(j + s1 + 1);
        uint32_t d = static_cast<uint32_t>(j + 1);
        idx.insert(idx.end(), {a, b, c, a, c, d});
    }

    // Caps
    uint32_t baseCenter = static_cast<uint32_t>(verts.size());
    verts.push_back({{0, 0, 0}, {0, -1, 0}});
    for (int j = 0; j < sectors; ++j) {
        float t0 = TWO_PI * static_cast<float>(j) / static_cast<float>(sectors);
        float t1 = TWO_PI * static_cast<float>(j + 1) / static_cast<float>(sectors);
        uint32_t v0 = static_cast<uint32_t>(verts.size());
        verts.push_back({{std::cos(t0) * radius, 0, std::sin(t0) * radius}, {0, -1, 0}});
        verts.push_back({{std::cos(t1) * radius, 0, std::sin(t1) * radius}, {0, -1, 0}});
        idx.insert(idx.end(), {baseCenter, v0 + 1, v0});
    }

    uint32_t topCenter = static_cast<uint32_t>(verts.size());
    verts.push_back({{0, height, 0}, {0, 1, 0}});
    for (int j = 0; j < sectors; ++j) {
        float t0 = TWO_PI * static_cast<float>(j) / static_cast<float>(sectors);
        float t1 = TWO_PI * static_cast<float>(j + 1) / static_cast<float>(sectors);
        uint32_t v0 = static_cast<uint32_t>(verts.size());
        verts.push_back({{std::cos(t0) * radius, height, std::sin(t0) * radius}, {0, 1, 0}});
        verts.push_back({{std::cos(t1) * radius, height, std::sin(t1) * radius}, {0, 1, 0}});
        idx.insert(idx.end(), {topCenter, v0, v0 + 1});
    }

    Mesh m;
    m.upload(verts, idx);
    return m;
}

// ---- Cône (axe Y, base à y=0, pointe à y=height) ----
Mesh Mesh::createCone(float radius, float height, int sectors) {
    std::vector<Vertex> verts;
    std::vector<uint32_t> idx;

    float slope = radius / height;

    // Pointe
    uint32_t tip = 0;
    verts.push_back({{0, height, 0}, {0, 1, 0}});

    // Ring de base
    for (int j = 0; j <= sectors; ++j) {
        float theta = TWO_PI * static_cast<float>(j) / static_cast<float>(sectors);
        float cosT = std::cos(theta), sinT = std::sin(theta);
        glm::vec3 n = glm::normalize(glm::vec3(cosT, slope, sinT));
        verts.push_back({{cosT * radius, 0, sinT * radius}, n});
    }

    for (int j = 0; j < sectors; ++j) {
        idx.insert(idx.end(), {tip,
                               static_cast<uint32_t>(j + 2),
                               static_cast<uint32_t>(j + 1)});
    }

    // Base cap
    uint32_t baseCenter = static_cast<uint32_t>(verts.size());
    verts.push_back({{0, 0, 0}, {0, -1, 0}});
    for (int j = 0; j < sectors; ++j) {
        float t0 = TWO_PI * static_cast<float>(j) / static_cast<float>(sectors);
        float t1 = TWO_PI * static_cast<float>(j + 1) / static_cast<float>(sectors);
        uint32_t v0 = static_cast<uint32_t>(verts.size());
        verts.push_back({{std::cos(t0) * radius, 0, std::sin(t0) * radius}, {0, -1, 0}});
        verts.push_back({{std::cos(t1) * radius, 0, std::sin(t1) * radius}, {0, -1, 0}});
        idx.insert(idx.end(), {baseCenter, v0 + 1, v0});
    }

    Mesh m;
    m.upload(verts, idx);
    return m;
}

// ---- Flèche = cylindre (shaft) + cône (head) ----
Mesh Mesh::createArrow(float shaftR, float shaftL, float headR, float headL, int sectors) {
    std::vector<Vertex> verts;
    std::vector<uint32_t> idx;

    auto addMesh = [&](const Mesh& /*unused*/) {};

    // Shaft (cylinder along Y from 0 to shaftL)
    for (int i = 0; i <= 1; ++i) {
        float y = static_cast<float>(i) * shaftL;
        for (int j = 0; j <= sectors; ++j) {
            float t = TWO_PI * static_cast<float>(j) / static_cast<float>(sectors);
            float c = std::cos(t), s = std::sin(t);
            verts.push_back({{c * shaftR, y, s * shaftR}, {c, 0, s}});
        }
    }
    int s1 = sectors + 1;
    for (int j = 0; j < sectors; ++j) {
        uint32_t a = static_cast<uint32_t>(j);
        uint32_t b = static_cast<uint32_t>(j + s1);
        uint32_t c = static_cast<uint32_t>(j + s1 + 1);
        uint32_t d = static_cast<uint32_t>(j + 1);
        idx.insert(idx.end(), {a, b, c, a, c, d});
    }

    // Head cone (from y=shaftL to y=shaftL+headL)
    uint32_t offset = static_cast<uint32_t>(verts.size());
    float slope = headR / headL;

    uint32_t tipIdx = offset;
    verts.push_back({{0, shaftL + headL, 0}, {0, 1, 0}});

    for (int j = 0; j <= sectors; ++j) {
        float t = TWO_PI * static_cast<float>(j) / static_cast<float>(sectors);
        float c = std::cos(t), s = std::sin(t);
        glm::vec3 n = glm::normalize(glm::vec3(c, slope, s));
        verts.push_back({{c * headR, shaftL, s * headR}, n});
    }

    for (int j = 0; j < sectors; ++j) {
        idx.insert(idx.end(), {tipIdx,
                               static_cast<uint32_t>(offset + 1 + j + 1),
                               static_cast<uint32_t>(offset + 1 + j)});
    }

    Mesh m;
    m.upload(verts, idx);
    return m;
}

// ---- Box (centré sur l'origine) ----
Mesh Mesh::createBox(float w, float h, float d) {
    float hw = w * 0.5f, hh = h * 0.5f, hd = d * 0.5f;

    std::vector<Vertex> verts = {
        // Front
        {{-hw,-hh, hd}, {0,0,1}}, {{ hw,-hh, hd}, {0,0,1}},
        {{ hw, hh, hd}, {0,0,1}}, {{-hw, hh, hd}, {0,0,1}},
        // Back
        {{ hw,-hh,-hd}, {0,0,-1}}, {{-hw,-hh,-hd}, {0,0,-1}},
        {{-hw, hh,-hd}, {0,0,-1}}, {{ hw, hh,-hd}, {0,0,-1}},
        // Top
        {{-hw, hh, hd}, {0,1,0}}, {{ hw, hh, hd}, {0,1,0}},
        {{ hw, hh,-hd}, {0,1,0}}, {{-hw, hh,-hd}, {0,1,0}},
        // Bottom
        {{-hw,-hh,-hd}, {0,-1,0}}, {{ hw,-hh,-hd}, {0,-1,0}},
        {{ hw,-hh, hd}, {0,-1,0}}, {{-hw,-hh, hd}, {0,-1,0}},
        // Right
        {{ hw,-hh, hd}, {1,0,0}}, {{ hw,-hh,-hd}, {1,0,0}},
        {{ hw, hh,-hd}, {1,0,0}}, {{ hw, hh, hd}, {1,0,0}},
        // Left
        {{-hw,-hh,-hd}, {-1,0,0}}, {{-hw,-hh, hd}, {-1,0,0}},
        {{-hw, hh, hd}, {-1,0,0}}, {{-hw, hh,-hd}, {-1,0,0}},
    };

    std::vector<uint32_t> idx;
    for (uint32_t face = 0; face < 6; ++face) {
        uint32_t base = face * 4;
        idx.insert(idx.end(), {base, base+1, base+2, base, base+2, base+3});
    }

    Mesh m;
    m.upload(verts, idx);
    return m;
}

// ---- Extrusion de contour le long d'un axe multi-stations ----
Mesh Mesh::createExtruded(
    const std::vector<glm::vec2>& contour,
    const std::vector<glm::vec2>& normals2D,
    const std::vector<glm::vec3>& axisPoints,
    const std::vector<glm::vec3>& axisUp,
    const std::vector<glm::vec3>& axisFwd)
{
    if (contour.empty() || axisPoints.size() < 2) return Mesh{};

    int nContour  = static_cast<int>(contour.size());
    int nStations = static_cast<int>(axisPoints.size());

    std::vector<Vertex> verts;
    std::vector<uint32_t> idx;
    verts.reserve(static_cast<size_t>(nContour * nStations));

    for (int s = 0; s < nStations; ++s) {
        glm::vec3 fwd = glm::normalize(axisFwd[s]);
        glm::vec3 up  = glm::normalize(axisUp[s]);
        glm::vec3 right = glm::normalize(glm::cross(fwd, up));
        up = glm::cross(right, fwd); // re-orthogonalise

        for (int c = 0; c < nContour; ++c) {
            glm::vec3 pos = axisPoints[s]
                          + right * contour[c].x
                          + up    * contour[c].y;
            glm::vec3 nrm = glm::normalize(right * normals2D[c].x + up * normals2D[c].y);
            verts.push_back({pos, nrm});
        }
    }

    // Connecter les tranches consécutives
    for (int s = 0; s < nStations - 1; ++s) {
        int ringA = s * nContour;
        int ringB = (s + 1) * nContour;
        for (int c = 0; c < nContour; ++c) {
            int cNext = (c + 1) % nContour;
            uint32_t a = static_cast<uint32_t>(ringA + c);
            uint32_t b = static_cast<uint32_t>(ringB + c);
            uint32_t bn = static_cast<uint32_t>(ringB + cNext);
            uint32_t an = static_cast<uint32_t>(ringA + cNext);
            idx.insert(idx.end(), {a, b, bn, a, bn, an});
        }
    }

    // Cap de début (face plane)
    if (nContour >= 3) {
        uint32_t base = static_cast<uint32_t>(verts.size());
        glm::vec3 capNormal = -glm::normalize(axisFwd[0]);
        glm::vec3 fwd0 = glm::normalize(axisFwd[0]);
        glm::vec3 up0  = glm::normalize(axisUp[0]);
        glm::vec3 right0 = glm::normalize(glm::cross(fwd0, up0));
        up0 = glm::cross(right0, fwd0);

        for (int c = 0; c < nContour; ++c) {
            glm::vec3 pos = axisPoints[0] + right0 * contour[c].x + up0 * contour[c].y;
            verts.push_back({pos, capNormal});
        }
        for (int c = 1; c < nContour - 1; ++c) {
            idx.insert(idx.end(), {base, static_cast<uint32_t>(base + c + 1), static_cast<uint32_t>(base + c)});
        }

        // Cap de fin
        uint32_t baseEnd = static_cast<uint32_t>(verts.size());
        glm::vec3 capNormalEnd = glm::normalize(axisFwd[nStations - 1]);
        int ls = nStations - 1;
        glm::vec3 fwdE = glm::normalize(axisFwd[ls]);
        glm::vec3 upE  = glm::normalize(axisUp[ls]);
        glm::vec3 rightE = glm::normalize(glm::cross(fwdE, upE));
        upE = glm::cross(rightE, fwdE);

        for (int c = 0; c < nContour; ++c) {
            glm::vec3 pos = axisPoints[ls] + rightE * contour[c].x + upE * contour[c].y;
            verts.push_back({pos, capNormalEnd});
        }
        for (int c = 1; c < nContour - 1; ++c) {
            idx.insert(idx.end(), {baseEnd, static_cast<uint32_t>(baseEnd + c), static_cast<uint32_t>(baseEnd + c + 1)});
        }
    }

    Mesh m;
    m.upload(verts, idx);
    return m;
}
