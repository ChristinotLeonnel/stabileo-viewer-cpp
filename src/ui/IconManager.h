#pragma once
// =============================================================================
//  IconManager.h — Gestionnaire d'Icônes Graphiques et Textures (Hazel Engine)
// =============================================================================

#include <GL/glew.h>
#include <imgui.h>
#include <string>
#include <unordered_map>

enum class IconType {
    Directory,
    FileGeneric,
    DxfModel,
    JsonModel,
    CSharpScript,
    Shader,
    PlaySimulation,
    PauseSimulation,
    ResetCamera,
    Settings
};

class IconManager {
public:
    /// Initialise et génère les textures d'icônes OpenGL haute définition (128x128).
    static void init();

    /// Libère toutes les textures d'icônes OpenGL.
    static void shutdown();

    /// Retourne l'identifiant de texture ImGui (ImTextureID) correspondant à un type d'icône.
    static ImTextureID getIcon(IconType type);

    /// Retourne automatiquement la bonne icône selon l'extension de fichier ou dossier.
    static ImTextureID getIconForPath(const std::string& extension, bool isDirectory);

    /// Retourne l'ID OpenGL brut de la texture.
    static GLuint getTextureId(IconType type);

private:
    static GLuint createFolderTexture();
    static GLuint createFileTexture();
    static GLuint createDxfTexture();
    static GLuint createJsonTexture();
    static GLuint createCSharpTexture();
    static GLuint createShaderTexture();
    static GLuint createPlayTexture();
    static GLuint createCameraTexture();

    static std::unordered_map<IconType, GLuint> s_icons;
    static bool s_initialized;
};
