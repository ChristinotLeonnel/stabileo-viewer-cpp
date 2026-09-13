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

    /// Zoom qui rapproche/éloigne le pivot (target) vers un point du monde
    /// (typiquement le point sous le curseur), pour un zoom "centré souris".
    void zoomToward(float delta, const glm::vec3& focusPoint);

    // ---- Vues prédéfinies complètes (Style Robot Structural Analysis) ----
    void setFrontView();      // Face (Z+)
    void setBackView();       // Arrière (Z-)
    void setTopView();        // Dessus / Plan (Y+)
    void setBottomView();     // Dessous (Y-)
    void setRightView();      // Droite (X+)
    void setLeftView();       // Gauche (X-)
    void setSideView() { setRightView(); } // Alias compatibilité
    void setIsometricView();  // Isométrique classique (SW)
    void setIsoCorner(int cornerIndex); // 0..7 pour chaque coin du ViewCube
    void rotateYaw(float deltaDeg); // Rotation en plan (+/-90°)
    
    // Animation fluide vers orientation cible
    void setOrientationSmooth(float targetYaw, float targetPitch);
    void update(float dt);

    /// Recadre la caméra pour que la boîte englobante soit visible.
    void fitToScene(const glm::vec3& sceneMin, const glm::vec3& sceneMax);

    bool  isAnimating = false;
    float animTargetYaw = 45.0f;
    float animTargetPitch = 25.0f;
};
