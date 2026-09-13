#pragma once
// =============================================================================
//  HazelUI.h — Composants et Thème d'Interface inspirés de Hazel Engine (The Cherno)
//
//  Contient :
//   - SetDarkThemeColors() : Le thème sombre iconique de Hazelnut Editor
//   - DrawVec3Control()    : Le widget X (Rouge), Y (Vert), Z (Bleu) de The Cherno
//   - DrawFloatControl()   : Contrôle numérique stylisé avec bouton reset
//   - BeginComponent()     : En-têtes de composants repliables avec bouton contextuel
// =============================================================================

#include <glm/glm.hpp>
#include <imgui.h>
#include <string>
#include <functional>

namespace Hazel::UI {

    /// Applique le thème sombre officiel de Hazelnut Editor (The Cherno).
    void SetDarkThemeColors();

    /// Le widget signature de The Cherno : Contrôle 3D avec boutons X (Rouge), Y (Vert), Z (Bleu).
    /// En cliquant sur X, Y ou Z, la coordonnée correspondante est réinitialisée à resetValue.
    bool DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 110.0f, float speed = 0.05f);

    /// Contrôle scalaire flottant avec étiquette et bouton de réinitialisation.
    bool DrawFloatControl(const std::string& label, float& value, float resetValue = 0.0f, float columnWidth = 110.0f, float speed = 0.05f, float min = 0.0f, float max = 0.0f, const char* format = "%.3f");

    /// Dessine un en-tête de composant Hazel avec bouton de configuration '...' et icône.
    bool BeginComponent(const std::string& name, bool defaultOpen = true);
    void EndComponent();

} // namespace Hazel::UI
