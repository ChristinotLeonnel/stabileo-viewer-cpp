// =============================================================================
//  DiagramMesh.cpp — Rubans 3D pour diagrammes d'efforts internes
// =============================================================================

#include "scene/DiagramMesh.h"
#include "scene/ProfileExtruder.h"
#include <algorithm>
#include <cmath>

namespace scene {

const std::vector<float>& getDiagramValues(
    const model::Element& element, DiagramType type)
{
    static const std::vector<float> empty;
    switch (type) {
        case DiagramType::N:  return element.N;
        case DiagramType::Vy: return element.Vy;
        case DiagramType::Vz: return element.Vz;
        case DiagramType::My: return element.My;
        case DiagramType::Mz: return element.Mz;
        case DiagramType::T:  return element.T;
        default: return empty;
    }
}

// Couleur du diagramme selon le type
static glm::vec3 diagramColor(DiagramType type) {
    switch (type) {
        case DiagramType::N:  return {0.2f, 0.7f, 0.9f}; // cyan
        case DiagramType::Vy: return {0.9f, 0.5f, 0.2f}; // orange
        case DiagramType::Vz: return {0.9f, 0.5f, 0.2f};
        case DiagramType::My: return {0.9f, 0.2f, 0.3f}; // rouge
        case DiagramType::Mz: return {0.9f, 0.2f, 0.3f};
        case DiagramType::T:  return {0.6f, 0.3f, 0.9f}; // violet
        default: return {0.8f, 0.8f, 0.8f};
    }
}

// Direction perpendiculaire pour le diagramme dans l'espace
static glm::vec3 diagramPerp(DiagramType type,
                              const glm::vec3& localY,
                              const glm::vec3& localZ) {
    switch (type) {
        case DiagramType::N:
        case DiagramType::Vy:
        case DiagramType::My:
            return localY;  // diagramme dans le plan de flexion principal
        case DiagramType::Vz:
        case DiagramType::Mz:
            return localZ;
        case DiagramType::T:
            return localY;
        default:
            return localY;
    }
}

Mesh createDiagramRibbon(
    const model::Element& element,
    const model::Node& nI, const model::Node& nJ,
    DiagramType type, float scale)
{
    const auto& values = getDiagramValues(element, type);
    const auto& stations = element.stations;
    if (values.empty() || stations.empty()) return Mesh{};

    glm::vec3 posI = nI.position;
    glm::vec3 posJ = nJ.position;
    glm::vec3 dir = posJ - posI;
    float L = glm::length(dir);
    if (L < 1e-6f) return Mesh{};

    glm::vec3 localX, localY, localZ;
    computeLocalFrame(posI, posJ, element.rollAngle, localX, localY, localZ);
    glm::vec3 perp = diagramPerp(type, localY, localZ);
    glm::vec3 color = diagramColor(type);

    int n = static_cast<int>(values.size());

    // Construire un ruban : 2 rangées de vertices
    // Rangée 0 : sur l'axe neutre
    // Rangée 1 : décalée par la valeur du diagramme
    std::vector<ColorVertex> verts;
    std::vector<uint32_t> idx;

    for (int i = 0; i < n; ++i) {
        float s = stations[static_cast<size_t>(i)];
        glm::vec3 axisPos = posI + dir * s;
        float val = values[static_cast<size_t>(i)] * scale;

        // Point sur l'axe neutre
        verts.push_back({axisPos, color});
        // Point décalé par la valeur
        verts.push_back({axisPos + perp * val, color});
    }

    // Triangles du ruban
    for (int i = 0; i < n - 1; ++i) {
        uint32_t a = static_cast<uint32_t>(i * 2);
        uint32_t b = a + 1;
        uint32_t c = a + 2;
        uint32_t d = a + 3;
        idx.insert(idx.end(), {a, c, d, a, d, b});
    }

    Mesh m;
    m.uploadColor(verts, idx);
    return m;
}

Mesh createDiagramOutline(
    const model::Element& element,
    const model::Node& nI, const model::Node& nJ,
    DiagramType type, float scale)
{
    const auto& values = getDiagramValues(element, type);
    const auto& stations = element.stations;
    if (values.empty() || stations.empty()) return Mesh{};

    glm::vec3 posI = nI.position;
    glm::vec3 posJ = nJ.position;
    glm::vec3 dir = posJ - posI;
    float L = glm::length(dir);
    if (L < 1e-6f) return Mesh{};

    glm::vec3 localX, localY, localZ;
    computeLocalFrame(posI, posJ, element.rollAngle, localX, localY, localZ);
    glm::vec3 perp = diagramPerp(type, localY, localZ);
    glm::vec3 color = diagramColor(type) * 1.2f;

    int n = static_cast<int>(values.size());

    std::vector<ColorVertex> verts;
    // Contour : axe neutre début -> valeurs -> axe neutre fin -> retour
    for (int i = 0; i < n; ++i) {
        float s = stations[static_cast<size_t>(i)];
        glm::vec3 axisPos = posI + dir * s;
        float val = values[static_cast<size_t>(i)] * scale;
        verts.push_back({axisPos + perp * val, color});
    }

    Mesh m;
    m.uploadLineStrip(verts);
    return m;
}

} // namespace scene
