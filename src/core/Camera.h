#pragma once
// =============================================================================
//  Camera.h — Caméra orbitale 3D (Arcball)
// =============================================================================

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    // ---- Paramètres d'orbite ----
    glm::vec3 target    = glm::vec3(0.0f);
    float     distance  = 20.0f;
    float     yaw       = 45.0f;     // degrés
    float     pitch     = 25.0f;     // degrés

    // ---- Projection ----
    float fov           = 45.0f;     // degrés
    float nearPlane     = 0.01f;
    float farPlane      = 2000.0f;
    float aspectRatio   = 16.0f / 9.0f;
    bool  orthographic  = false;
    float orthoScale    = 10.0f;

    // ---- Sensibilité ----
    float rotateSens    = 0.25f;
    float panSens       = 0.008f;
    float zoomSens      = 1.2f;

    // ---- Matrices ----
    glm::vec3 getPosition()         const;
    glm::mat4 getViewMatrix()       const;
    glm::mat4 getProjectionMatrix() const;

    // ---- Contrôles souris ----
    void rotate(float dx, float dy);
    void pan   (float dx, float dy);
    void zoom  (float delta);

    // ---- Vues prédéfinies ----
    void setFrontView();      // X-Z
    void setTopView();        // X-Y (vue de dessus)
    void setSideView();       // Y-Z
    void setIsometricView();  // Isométrique classique

    /// Recadre la caméra pour que la boîte englobante soit visible.
    void fitToScene(const glm::vec3& sceneMin, const glm::vec3& sceneMax);
};
