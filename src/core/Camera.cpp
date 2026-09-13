// =============================================================================
//  Camera.cpp — Caméra orbitale 3D
// =============================================================================

#include "core/Camera.h"
#include <algorithm>
#include <cmath>

glm::vec3 Camera::getPosition() const {
    float yawR   = glm::radians(yaw);
    float pitchR = glm::radians(pitch);
    float cosPitch = std::cos(pitchR);
    return target + glm::vec3(
        distance * cosPitch * std::sin(yawR),
        distance * std::sin(pitchR),
        distance * cosPitch * std::cos(yawR)
    );
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(getPosition(), target, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 Camera::getProjectionMatrix() const {
    if (orthographic) {
        float hw = orthoScale * aspectRatio;
        float hh = orthoScale;
        return glm::ortho(-hw, hw, -hh, hh, nearPlane, farPlane);
    }
    return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
}

void Camera::rotate(float dx, float dy) {
    yaw   += dx * rotateSens;
    pitch += dy * rotateSens;
    pitch = std::clamp(pitch, -89.0f, 89.0f);
}

void Camera::pan(float dx, float dy) {
    float yawR   = glm::radians(yaw);
    float pitchR = glm::radians(pitch);
    glm::vec3 right(std::cos(yawR), 0.0f, -std::sin(yawR));
    // BUGFIX: le vecteur "haut" réel de la caméra dépend aussi du pitch,
    // pas seulement de l'axe Y du monde. Avec l'ancien code, dès que la
    // caméra était inclinée (cas par défaut : pitch = 25°), le panoramique
    // vertical dérivait en diagonale au lieu de suivre le curseur.
    glm::vec3 up(
        -std::sin(yawR) * std::sin(pitchR),
         std::cos(pitchR),
        -std::cos(yawR) * std::sin(pitchR)
    );
    target -= right * dx * panSens * distance;
    target += up    * dy * panSens * distance;
}

void Camera::zoom(float delta) {
    distance *= (delta > 0) ? (1.0f / zoomSens) : zoomSens;
    distance = std::clamp(distance, 0.1f, 1000.0f);
    orthoScale = distance * 0.5f;
}

void Camera::zoomToward(float delta, const glm::vec3& focusPoint) {
    float oldDistance = distance;
    zoom(delta);
    // Déplace le pivot vers le point visé, proportionnellement au
    // rapprochement effectif, pour donner la sensation d'un zoom
    // centré sur le curseur plutôt que sur le pivot fixe.
    float t = 1.0f - (distance / oldDistance);
    target += (focusPoint - target) * t;
}

void Camera::setFrontView() {
    setOrientationSmooth(0.0f, 0.0f);
}

void Camera::setBackView() {
    setOrientationSmooth(180.0f, 0.0f);
}

void Camera::setTopView() {
    setOrientationSmooth(0.0f, 89.0f);
}

void Camera::setBottomView() {
    setOrientationSmooth(0.0f, -89.0f);
}

void Camera::setRightView() {
    setOrientationSmooth(90.0f, 0.0f);
}

void Camera::setLeftView() {
    setOrientationSmooth(-90.0f, 0.0f);
}

void Camera::setIsometricView() {
    setOrientationSmooth(45.0f, 30.0f);
}

void Camera::setIsoCorner(int cornerIndex) {
    // 8 coins du ViewCube (Top 4, Bottom 4)
    switch (cornerIndex) {
        case 0: setOrientationSmooth(45.0f, 35.0f); break;   // Front-Right-Top
        case 1: setOrientationSmooth(-45.0f, 35.0f); break;  // Front-Left-Top
        case 2: setOrientationSmooth(135.0f, 35.0f); break;  // Back-Right-Top
        case 3: setOrientationSmooth(-135.0f, 35.0f); break; // Back-Left-Top
        case 4: setOrientationSmooth(45.0f, -35.0f); break;  // Front-Right-Bottom
        case 5: setOrientationSmooth(-45.0f, -35.0f); break; // Front-Left-Bottom
        case 6: setOrientationSmooth(135.0f, -35.0f); break; // Back-Right-Bottom
        case 7: setOrientationSmooth(-135.0f, -35.0f); break;// Back-Left-Bottom
        default: setIsometricView(); break;
    }
}

void Camera::rotateYaw(float deltaDeg) {
    setOrientationSmooth(yaw + deltaDeg, pitch);
}

void Camera::setOrientationSmooth(float targetY, float targetP) {
    targetP = std::clamp(targetP, -89.0f, 89.0f);
    // Normaliser l'angle yaw le plus court
    while (targetY - yaw > 180.0f) targetY -= 360.0f;
    while (targetY - yaw < -180.0f) targetY += 360.0f;

    animTargetYaw = targetY;
    animTargetPitch = targetP;
    isAnimating = true;
}

void Camera::update(float dt) {
    if (!isAnimating) return;

    float speed = 12.0f; // Vitesse de convergence fluide
    float factor = std::clamp(speed * dt, 0.0f, 1.0f);

    yaw   += (animTargetYaw - yaw) * factor;
    pitch += (animTargetPitch - pitch) * factor;

    if (std::abs(animTargetYaw - yaw) < 0.05f && std::abs(animTargetPitch - pitch) < 0.05f) {
        yaw = animTargetYaw;
        pitch = animTargetPitch;
        isAnimating = false;
    }
}

void Camera::fitToScene(const glm::vec3& sceneMin, const glm::vec3& sceneMax) {
    target = (sceneMin + sceneMax) * 0.5f;
    float diag = glm::length(sceneMax - sceneMin);
    distance = std::max(diag * 1.5f, 2.0f);
    orthoScale = distance * 0.5f;
}
