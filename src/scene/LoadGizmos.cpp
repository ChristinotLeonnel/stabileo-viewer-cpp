// =============================================================================
//  LoadGizmos.cpp — Flèches 3D de chargement
// =============================================================================

#include "scene/LoadGizmos.h"
#include <glm/gtc/constants.hpp>
#include <cmath>
#include <algorithm>

namespace scene {

Mesh createForceArrow(const glm::vec3& direction, float magnitude, float maxMag) {
    float normMag = std::min(std::abs(magnitude) / std::max(maxMag, 0.01f), 1.0f);
    float arrowLen = 0.5f + normMag * 1.5f;

    return Mesh::createArrow(0.02f, arrowLen * 0.75f, 0.06f, arrowLen * 0.25f, 10);
}

Mesh createMomentArc(const glm::vec3& axis, float magnitude, float maxMag) {
    (void)axis;
    // Arc de cercle simplifié : série de segments de ligne
    std::vector<ColorVertex> verts;
    float radius = 0.3f;
    float normMag = std::min(std::abs(magnitude) / std::max(maxMag, 0.01f), 1.0f);
    float arcAngle = glm::pi<float>() * (0.5f + normMag * 1.0f);
    int segments = 20;
    glm::vec3 color(1.0f, 0.6f, 0.1f);

    for (int i = 0; i <= segments; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(segments);
        float angle = -arcAngle * 0.5f + t * arcAngle;
        float x = std::cos(angle) * radius;
        float z = std::sin(angle) * radius;
        verts.push_back({{x, 0, z}, color});
    }

    Mesh m;
    m.uploadLineStrip(verts);
    return m;
}

Mesh createDistributedLoadComb(
    const model::DistributedLoad& load,
    const glm::vec3& posI, const glm::vec3& posJ,
    const glm::vec3& localY, const glm::vec3& localZ,
    float maxMag, int numArrows)
{
    std::vector<Vertex> verts;
    std::vector<uint32_t> idx;

    glm::vec3 dir = posJ - posI;
    float L = glm::length(dir);
    if (L < 1e-6f) return Mesh{};

    // Direction de la charge (en coordonnées globales, approximé)
    glm::vec3 loadDir = glm::normalize(
        localY * load.wStart.y + localZ * load.wStart.x
    );
    if (glm::length(loadDir) < 0.01f) loadDir = glm::vec3(0, -1, 0);

    for (int i = 0; i < numArrows; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(numArrows - 1);
        glm::vec3 pos = posI + dir * t;

        // Intensité interpolée
        float w = glm::length(glm::mix(load.wStart, load.wEnd, t));
        float normW = std::min(w / std::max(maxMag, 0.01f), 1.0f);
        float arrowH = 0.3f + normW * 1.2f;

        // Petite flèche (triangle aplati)
        glm::vec3 tip = pos;
        glm::vec3 base = pos - loadDir * arrowH;
        glm::vec3 perp = glm::normalize(glm::cross(loadDir, dir));
        if (glm::length(perp) < 0.01f) perp = glm::vec3(1, 0, 0);

        float hw = 0.04f;
        uint32_t off = static_cast<uint32_t>(verts.size());
        glm::vec3 n = glm::normalize(glm::cross(perp, loadDir));

        verts.push_back({tip, n});
        verts.push_back({base + perp * hw, n});
        verts.push_back({base - perp * hw, n});

        idx.insert(idx.end(), {off, off + 1, off + 2});

        // Tige (ligne simplifiée comme quad mince)
        uint32_t off2 = static_cast<uint32_t>(verts.size());
        glm::vec3 stemTop = base;
        glm::vec3 stemBot = base - loadDir * 0.05f;
        float sw = 0.015f;
        verts.push_back({stemTop + perp * sw, n});
        verts.push_back({stemTop - perp * sw, n});
        verts.push_back({stemBot - perp * sw, n});
        verts.push_back({stemBot + perp * sw, n});
        idx.insert(idx.end(), {off2, off2+1, off2+2, off2, off2+2, off2+3});
    }

    // Ligne de raccord entre sommets des flèches
    // (ajoutée comme quads fins)
    for (int i = 0; i < numArrows - 1; ++i) {
        float t0 = static_cast<float>(i) / static_cast<float>(numArrows - 1);
        float t1 = static_cast<float>(i + 1) / static_cast<float>(numArrows - 1);
        glm::vec3 p0 = posI + dir * t0;
        glm::vec3 p1 = posI + dir * t1;

        float w0 = glm::length(glm::mix(load.wStart, load.wEnd, t0));
        float w1 = glm::length(glm::mix(load.wStart, load.wEnd, t1));
        float h0 = 0.3f + std::min(w0 / std::max(maxMag, 0.01f), 1.0f) * 1.2f;
        float h1 = 0.3f + std::min(w1 / std::max(maxMag, 0.01f), 1.0f) * 1.2f;

        glm::vec3 top0 = p0 - loadDir * h0;
        glm::vec3 top1 = p1 - loadDir * h1;
        glm::vec3 perp = glm::normalize(glm::cross(loadDir, dir));
        if (glm::length(perp) < 0.01f) perp = glm::vec3(1, 0, 0);
        float lw = 0.012f;
        glm::vec3 n = glm::normalize(glm::cross(perp, loadDir));

        uint32_t off = static_cast<uint32_t>(verts.size());
        verts.push_back({top0 + perp * lw, n});
        verts.push_back({top0 - perp * lw, n});
        verts.push_back({top1 - perp * lw, n});
        verts.push_back({top1 + perp * lw, n});
        idx.insert(idx.end(), {off, off+1, off+2, off, off+2, off+3});
    }

    Mesh m;
    m.upload(verts, idx);
    return m;
}

} // namespace scene
