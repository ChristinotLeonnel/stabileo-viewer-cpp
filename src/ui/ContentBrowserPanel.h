#pragma once
// =============================================================================
//  ContentBrowserPanel.h — Explorateur d'Assets inspiré de Hazel Engine (The Cherno)
// =============================================================================

#include <filesystem>
#include <string>
#include <vector>

class ContentBrowserPanel {
public:
    ContentBrowserPanel();

    void onImGuiRender();

    // Signaux d'actions utilisateur
    std::string getPendingLoadFile() {
        std::string result = pendingLoadFile_;
        pendingLoadFile_.clear();
        return result;
    }

    std::string getPendingDxfFile() {
        std::string result = pendingDxfFile_;
        pendingDxfFile_.clear();
        return result;
    }

    std::string getPendingScriptFile() {
        std::string result = pendingScriptFile_;
        pendingScriptFile_.clear();
        return result;
    }

private:
    std::filesystem::path baseDirectory_;
    std::filesystem::path currentDirectory_;

    std::string pendingLoadFile_;
    std::string pendingDxfFile_;
    std::string pendingScriptFile_;

    float thumbnailSize_ = 72.0f;
    float padding_ = 12.0f;
};
