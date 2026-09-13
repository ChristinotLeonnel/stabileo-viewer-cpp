// =============================================================================
//  IconManager.cpp — Gestionnaire d'Icônes Graphiques et Textures (Hazel Engine)
// =============================================================================

#include "ui/IconManager.h"
#include <vector>
#include <cmath>
#include <algorithm>

std::unordered_map<IconType, GLuint> IconManager::s_icons;
bool IconManager::s_initialized = false;

namespace {

    struct PixelCanvas {
        int width = 128;
        int height = 128;
        std::vector<uint8_t> data;

        PixelCanvas(int w = 128, int h = 128) : width(w), height(h), data(w * h * 4, 0) {}

        void setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
            if (x < 0 || x >= width || y < 0 || y >= height || a == 0) return;
            int idx = (y * width + x) * 4;
            if (a == 255 || data[idx + 3] == 0) {
                data[idx + 0] = r;
                data[idx + 1] = g;
                data[idx + 2] = b;
                data[idx + 3] = a;
            } else {
                float alpha = a / 255.0f;
                float invAlpha = 1.0f - alpha;
                data[idx + 0] = (uint8_t)(r * alpha + data[idx + 0] * invAlpha);
                data[idx + 1] = (uint8_t)(g * alpha + data[idx + 1] * invAlpha);
                data[idx + 2] = (uint8_t)(b * alpha + data[idx + 2] * invAlpha);
                data[idx + 3] = std::max(data[idx + 3], a);
            }
        }

        void fillRect(int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
            for (int y = std::max(0, y0); y <= std::min(height - 1, y1); ++y) {
                for (int x = std::max(0, x0); x <= std::min(width - 1, x1); ++x) {
                    setPixel(x, y, r, g, b, a);
                }
            }
        }

        void fillRoundedRect(int x0, int y0, int x1, int y1, int radius, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
            for (int y = std::max(0, y0); y <= std::min(height - 1, y1); ++y) {
                for (int x = std::max(0, x0); x <= std::min(width - 1, x1); ++x) {
                    int dx = 0, dy = 0;
                    if (x < x0 + radius) dx = (x0 + radius) - x;
                    else if (x > x1 - radius) dx = x - (x1 - radius);

                    if (y < y0 + radius) dy = (y0 + radius) - y;
                    else if (y > y1 - radius) dy = y - (y1 - radius);

                    if (dx > 0 && dy > 0) {
                        float dist = std::sqrt((float)(dx * dx + dy * dy));
                        if (dist <= radius - 0.5f) {
                            setPixel(x, y, r, g, b, a);
                        } else if (dist <= radius + 0.5f) {
                            float edgeAlpha = 1.0f - (dist - (radius - 0.5f));
                            setPixel(x, y, r, g, b, (uint8_t)(a * edgeAlpha));
                        }
                    } else {
                        setPixel(x, y, r, g, b, a);
                    }
                }
            }
        }

        void fillCircle(int cx, int cy, int radius, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
            int x0 = cx - radius - 1;
            int x1 = cx + radius + 1;
            int y0 = cy - radius - 1;
            int y1 = cy + radius + 1;
            for (int y = std::max(0, y0); y <= std::min(height - 1, y1); ++y) {
                for (int x = std::max(0, x0); x <= std::min(width - 1, x1); ++x) {
                    int dx = x - cx;
                    int dy = y - cy;
                    float dist = std::sqrt((float)(dx * dx + dy * dy));
                    if (dist <= radius - 0.5f) {
                        setPixel(x, y, r, g, b, a);
                    } else if (dist <= radius + 0.5f) {
                        float edgeAlpha = 1.0f - (dist - (radius - 0.5f));
                        setPixel(x, y, r, g, b, (uint8_t)(a * edgeAlpha));
                    }
                }
            }
        }

        void drawLine(int x0, int y0, int x1, int y1, int thickness, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
            int dx = std::abs(x1 - x0);
            int dy = std::abs(y1 - y0);
            int sx = x0 < x1 ? 1 : -1;
            int sy = y0 < y1 ? 1 : -1;
            int err = dx - dy;

            int halfT = thickness / 2;
            int cx = x0, cy = y0;
            while (true) {
                for (int ty = -halfT; ty <= halfT; ++ty) {
                    for (int tx = -halfT; tx <= halfT; ++tx) {
                        setPixel(cx + tx, cy + ty, r, g, b, a);
                    }
                }
                if (cx == x1 && cy == y1) break;
                int e2 = 2 * err;
                if (e2 > -dy) { err -= dy; cx += sx; }
                if (e2 < dx)  { err += dx; cy += sy; }
            }
        }

        GLuint uploadToOpenGL() const {
            GLuint texId = 0;
            glGenTextures(1, &texId);
            glBindTexture(GL_TEXTURE_2D, texId);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
            glBindTexture(GL_TEXTURE_2D, 0);
            return texId;
        }
    };

} // namespace

void IconManager::init() {
    if (s_initialized) return;

    s_icons[IconType::Directory]       = createFolderTexture();
    s_icons[IconType::FileGeneric]     = createFileTexture();
    s_icons[IconType::DxfModel]        = createDxfTexture();
    s_icons[IconType::JsonModel]       = createJsonTexture();
    s_icons[IconType::CSharpScript]    = createCSharpTexture();
    s_icons[IconType::Shader]          = createShaderTexture();
    s_icons[IconType::PlaySimulation]  = createPlayTexture();
    s_icons[IconType::ResetCamera]     = createCameraTexture();

    s_initialized = true;
}

void IconManager::shutdown() {
    for (auto& pair : s_icons) {
        if (pair.second != 0) {
            glDeleteTextures(1, &pair.second);
        }
    }
    s_icons.clear();
    s_initialized = false;
}

ImTextureID IconManager::getIcon(IconType type) {
    auto it = s_icons.find(type);
    if (it != s_icons.end()) {
        return (ImTextureID)(intptr_t)it->second;
    }
    return (ImTextureID)0;
}

GLuint IconManager::getTextureId(IconType type) {
    auto it = s_icons.find(type);
    if (it != s_icons.end()) {
        return it->second;
    }
    return 0;
}

ImTextureID IconManager::getIconForPath(const std::string& extension, bool isDirectory) {
    if (isDirectory) {
        return getIcon(IconType::Directory);
    }

    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(::tolower(c)); });

    if (ext == ".dxf") {
        return getIcon(IconType::DxfModel);
    } else if (ext == ".json") {
        return getIcon(IconType::JsonModel);
    } else if (ext == ".cs") {
        return getIcon(IconType::CSharpScript);
    } else if (ext == ".glsl" || ext == ".vert" || ext == ".frag") {
        return getIcon(IconType::Shader);
    }

    return getIcon(IconType::FileGeneric);
}

// -----------------------------------------------------------------------------
//  1. Icône Dossier (Hazel Engine Amber Folder)
// -----------------------------------------------------------------------------
GLuint IconManager::createFolderTexture() {
    PixelCanvas canvas;

    // Onglet arrière du dossier
    canvas.fillRoundedRect(18, 22, 58, 42, 6, 215, 140, 30, 255);

    // Corps arrière
    canvas.fillRoundedRect(16, 32, 112, 106, 8, 225, 150, 35, 255);

    // Document blanc légèrement sorti du dossier pour donner vie à l'icône
    canvas.fillRoundedRect(28, 20, 100, 50, 4, 240, 245, 250, 240);
    canvas.fillRect(36, 28, 70, 32, 170, 190, 210, 220);
    canvas.fillRect(36, 36, 88, 40, 170, 190, 210, 220);

    // Rabat avant du dossier avec dégradé chaud
    for (int y = 42; y <= 108; ++y) {
        float t = (float)(y - 42) / (108.0f - 42.0f);
        uint8_t r = (uint8_t)(255 * (1.0f - t * 0.15f));
        uint8_t g = (uint8_t)(185 * (1.0f - t * 0.18f));
        uint8_t b = (uint8_t)(45 * (1.0f - t * 0.10f));
        canvas.fillRect(16, y, 112, y, r, g, b, 255);
    }
    // Bordures arrondies du rabat
    canvas.fillRoundedRect(16, 42, 112, 108, 8, 245, 175, 40, 255);

    // Ligne lumineuse en haut du rabat
    canvas.drawLine(22, 44, 106, 44, 2, 255, 220, 130, 220);

    return canvas.uploadToOpenGL();
}

// -----------------------------------------------------------------------------
//  2. Icône Fichier Générique (Document avec coin replié)
// -----------------------------------------------------------------------------
GLuint IconManager::createFileTexture() {
    PixelCanvas canvas;

    // Ombre portée subtile
    canvas.fillRoundedRect(26, 16, 104, 116, 8, 0, 0, 0, 70);

    // Corps principal de la page
    canvas.fillRoundedRect(24, 14, 102, 114, 8, 238, 242, 245, 255);

    // Coin replié en haut à droite (triangle / origami)
    canvas.fillRect(74, 14, 102, 42, 0, 0, 0, 0); // Vider le coin
    // Flap replié
    canvas.fillRoundedRect(74, 14, 102, 42, 4, 190, 200, 208, 255);
    canvas.drawLine(74, 14, 102, 42, 2, 160, 172, 182, 255);

    // Lignes de contenu du document
    canvas.fillRect(36, 52, 70, 56, 120, 140, 155, 255);
    canvas.fillRect(36, 64, 90, 68, 160, 175, 185, 255);
    canvas.fillRect(36, 76, 84, 80, 160, 175, 185, 255);
    canvas.fillRect(36, 88, 76, 92, 160, 175, 185, 255);
    canvas.fillRect(36, 100, 60, 104, 160, 175, 185, 255);

    return canvas.uploadToOpenGL();
}

// -----------------------------------------------------------------------------
//  3. Icône Fichier DXF / CAO (AutoCAD Blueprint & Drafting)
// -----------------------------------------------------------------------------
GLuint IconManager::createDxfTexture() {
    PixelCanvas canvas;

    // Corps du document
    canvas.fillRoundedRect(24, 14, 102, 114, 8, 242, 245, 248, 255);

    // En-tête rouge-orange architectural (Couleur Autodesk DXF)
    canvas.fillRect(24, 14, 102, 42, 229, 57, 53, 255);

    // Symbole de compas et tracé géométrique au centre
    canvas.drawLine(40, 92, 64, 52, 3, 211, 47, 47, 255);
    canvas.drawLine(64, 52, 88, 92, 3, 211, 47, 47, 255);
    canvas.drawLine(50, 76, 78, 76, 2, 211, 47, 47, 255);
    canvas.fillCircle(64, 52, 5, 229, 57, 53, 255);

    // Badge "DXF" distinctif en bas
    canvas.fillRoundedRect(34, 96, 92, 110, 4, 198, 40, 40, 255);
    // Tracé stylisé du mot "DXF"
    canvas.drawLine(42, 99, 42, 107, 2, 255, 255, 255, 255); // D
    canvas.drawLine(42, 99, 48, 103, 2, 255, 255, 255, 255);
    canvas.drawLine(48, 103, 42, 107, 2, 255, 255, 255, 255);

    canvas.drawLine(56, 99, 66, 107, 2, 255, 255, 255, 255); // X
    canvas.drawLine(66, 99, 56, 107, 2, 255, 255, 255, 255);

    canvas.drawLine(74, 99, 74, 107, 2, 255, 255, 255, 255); // F
    canvas.drawLine(74, 99, 82, 99, 2, 255, 255, 255, 255);
    canvas.drawLine(74, 103, 80, 103, 2, 255, 255, 255, 255);

    return canvas.uploadToOpenGL();
}

// -----------------------------------------------------------------------------
//  4. Icône Modèle JSON / FEM (Structure Éléments Finis 3D)
// -----------------------------------------------------------------------------
GLuint IconManager::createJsonTexture() {
    PixelCanvas canvas;

    // Corps du document
    canvas.fillRoundedRect(24, 14, 102, 114, 8, 240, 248, 242, 255);

    // En-tête vert émeraude (Modèles & Solveur)
    canvas.fillRect(24, 14, 102, 42, 46, 125, 50, 255);

    // Treillis spatial isométrique (Cube filaire 3D)
    // Face avant
    canvas.drawLine(44, 62, 84, 62, 2, 46, 125, 50, 255);
    canvas.drawLine(44, 86, 84, 86, 2, 46, 125, 50, 255);
    canvas.drawLine(44, 62, 44, 86, 2, 46, 125, 50, 255);
    canvas.drawLine(84, 62, 84, 86, 2, 46, 125, 50, 255);
    // Diagonale de contreventement
    canvas.drawLine(44, 86, 84, 62, 2, 76, 175, 80, 255);

    // Nœuds du treillis (Sphères vertes)
    canvas.fillCircle(44, 62, 4, 27, 94, 32, 255);
    canvas.fillCircle(84, 62, 4, 27, 94, 32, 255);
    canvas.fillCircle(44, 86, 4, 27, 94, 32, 255);
    canvas.fillCircle(84, 86, 4, 27, 94, 32, 255);

    // Badge "JSON" vert
    canvas.fillRoundedRect(32, 96, 94, 110, 4, 38, 105, 42, 255);
    canvas.fillRect(40, 101, 86, 105, 255, 255, 255, 255);

    return canvas.uploadToOpenGL();
}

// -----------------------------------------------------------------------------
//  5. Icône Script C# (Hazel Engine / Unity Purple Shield)
// -----------------------------------------------------------------------------
GLuint IconManager::createCSharpTexture() {
    PixelCanvas canvas;

    // Corps du document
    canvas.fillRoundedRect(24, 14, 102, 114, 8, 245, 240, 248, 255);

    // En-tête violet C# officiel (.NET CoreCLR)
    canvas.fillRect(24, 14, 102, 42, 123, 31, 162, 255);

    // Grand symbole C# au centre
    // Lettre 'C'
    canvas.drawLine(46, 56, 46, 88, 4, 123, 31, 162, 255);
    canvas.drawLine(46, 56, 62, 56, 4, 123, 31, 162, 255);
    canvas.drawLine(46, 88, 62, 88, 4, 123, 31, 162, 255);

    // Symbole '#'
    canvas.drawLine(72, 56, 70, 88, 3, 156, 39, 176, 255);
    canvas.drawLine(80, 56, 78, 88, 3, 156, 39, 176, 255);
    canvas.drawLine(66, 66, 86, 66, 3, 156, 39, 176, 255);
    canvas.drawLine(64, 78, 84, 78, 3, 156, 39, 176, 255);

    // Badge "C#" en bas
    canvas.fillRoundedRect(36, 96, 90, 110, 4, 106, 27, 140, 255);
    canvas.fillRect(44, 101, 82, 105, 255, 255, 255, 255);

    return canvas.uploadToOpenGL();
}

// -----------------------------------------------------------------------------
//  6. Icône Shader GLSL (Diamant étincelant)
// -----------------------------------------------------------------------------
GLuint IconManager::createShaderTexture() {
    PixelCanvas canvas;

    canvas.fillRoundedRect(24, 14, 102, 114, 8, 240, 248, 252, 255);
    canvas.fillRect(24, 14, 102, 42, 0, 151, 167, 255);

    // Diamant de shader au centre
    canvas.drawLine(64, 52, 86, 72, 3, 0, 172, 193, 255);
    canvas.drawLine(86, 72, 64, 92, 3, 0, 172, 193, 255);
    canvas.drawLine(64, 92, 42, 72, 3, 0, 172, 193, 255);
    canvas.drawLine(42, 72, 64, 52, 3, 0, 172, 193, 255);
    canvas.drawLine(42, 72, 86, 72, 2, 77, 208, 225, 255);

    canvas.fillCircle(64, 72, 5, 0, 151, 167, 255);

    return canvas.uploadToOpenGL();
}

// -----------------------------------------------------------------------------
//  7. Icône Play / Résolution EF
// -----------------------------------------------------------------------------
GLuint IconManager::createPlayTexture() {
    PixelCanvas canvas;
    canvas.fillCircle(64, 64, 54, 46, 125, 50, 255);
    canvas.fillCircle(64, 64, 48, 76, 175, 80, 255);

    // Triangle Play
    for (int x = 48; x <= 86; ++x) {
        float progress = (float)(x - 48) / (86.0f - 48.0f);
        int halfH = (int)(26 * (1.0f - progress));
        canvas.fillRect(x, 64 - halfH, x, 64 + halfH, 255, 255, 255, 255);
    }

    return canvas.uploadToOpenGL();
}

// -----------------------------------------------------------------------------
//  8. Icône Caméra 3D
// -----------------------------------------------------------------------------
GLuint IconManager::createCameraTexture() {
    PixelCanvas canvas;
    canvas.fillCircle(64, 64, 54, 30, 136, 229, 255);
    canvas.fillCircle(64, 64, 48, 66, 165, 245, 255);

    // Boîtier appareil / caméra
    canvas.fillRoundedRect(42, 50, 86, 80, 6, 255, 255, 255, 255);
    canvas.fillRoundedRect(56, 44, 72, 50, 3, 255, 255, 255, 255);
    canvas.fillCircle(64, 65, 10, 30, 136, 229, 255);

    return canvas.uploadToOpenGL();
}
