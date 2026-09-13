// =============================================================================
//  Grid.cpp — Grille de référence au sol et trièdre d'orientation
// =============================================================================

#include "core/Grid.h"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

void Grid::init(float halfSize, float spacing) {
    std::vector<ColorVertex> verts;
    glm::vec3 gray(0.35f, 0.35f, 0.38f);
    glm::vec3 dark(0.22f, 0.22f, 0.25f);

    // Lignes parallèles à X (variation de Z)
    for (float z = -halfSize; z <= halfSize; z += spacing) {
        glm::vec3 c = (std::abs(z) < 0.01f) ? glm::vec3(0.3f, 0.3f, 0.8f) : dark;
        verts.push_back({{-halfSize, 0, z}, c});
        verts.push_back({{ halfSize, 0, z}, c});
    }
    // Lignes parallèles à Z (variation de X)
    for (float x = -halfSize; x <= halfSize; x += spacing) {
        glm::vec3 c = (std::abs(x) < 0.01f) ? glm::vec3(0.8f, 0.3f, 0.3f) : dark;
        verts.push_back({{x, 0, -halfSize}, c});
        verts.push_back({{x, 0,  halfSize}, c});
    }

    gridMesh_.uploadLines(verts);
}

void Grid::draw(const Shader& shader) const {
    gridMesh_.draw();
}

void Grid::initAxesTriad() {
    std::vector<ColorVertex> verts;
    float len = 1.0f;
    glm::vec3 r(1.0f, 0.2f, 0.2f);
    glm::vec3 g(0.2f, 1.0f, 0.2f);
    glm::vec3 b(0.3f, 0.4f, 1.0f);

    // X axis
    verts.push_back({{0, 0, 0}, r});
    verts.push_back({{len, 0, 0}, r});
    // Y axis
    verts.push_back({{0, 0, 0}, g});
    verts.push_back({{0, len, 0}, g});
    // Z axis
    verts.push_back({{0, 0, 0}, b});
    verts.push_back({{0, 0, len}, b});

    axesMesh_.uploadLines(verts);
}

void Grid::drawAxesTriad(const Shader& shader,
                         const glm::mat4& viewRotOnly,
                         float vpW, float vpH) const
{
    // Dessiner dans un petit viewport en bas à gauche
    float sz = 80.0f;
    glViewport(10, 10, static_cast<GLsizei>(sz), static_cast<GLsizei>(sz));

    // Projection orthographique tight
    float orthoHalf = 1.5f;
    glm::mat4 proj = glm::ortho(-orthoHalf, orthoHalf, -orthoHalf, orthoHalf, -10.0f, 10.0f);
    glm::mat4 model(1.0f);

    shader.use();
    shader.setMat4("uView", viewRotOnly);
    shader.setMat4("uProjection", proj);
    shader.setMat4("uModel", model);
    shader.setFloat("uAlpha", 1.0f);

    glLineWidth(2.5f);
    axesMesh_.draw();
    glLineWidth(1.0f);

    // Restaurer le viewport principal
    glViewport(0, 0, static_cast<GLsizei>(vpW), static_cast<GLsizei>(vpH));
}
