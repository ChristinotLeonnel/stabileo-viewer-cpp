// =============================================================================
//  ContentBrowserPanel.cpp — Explorateur d'Assets inspiré de Hazel Engine (The Cherno)
// =============================================================================

#include "ui/ContentBrowserPanel.h"
#include <imgui.h>
#include <algorithm>
#include <system_error>

namespace fs = std::filesystem;

ContentBrowserPanel::ContentBrowserPanel() {
    // Recherche du dossier assets/
    if (fs::exists("assets")) {
        baseDirectory_ = fs::canonical("assets");
    } else if (fs::exists("../assets")) {
        baseDirectory_ = fs::canonical("../assets");
    } else if (fs::exists("../../assets")) {
        baseDirectory_ = fs::canonical("../../assets");
    } else {
        baseDirectory_ = fs::current_path();
    }
    currentDirectory_ = baseDirectory_;
}

void ContentBrowserPanel::onImGuiRender() {
    ImGui::Begin("Explorateur de Contenu (Assets)###ContentBrowser");

    // Bouton Retour de style Hazel
    bool canGoBack = (currentDirectory_ != baseDirectory_);
    if (!canGoBack) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("<##BackBtn", ImVec2(28.0f, 24.0f))) {
        currentDirectory_ = currentDirectory_.parent_path();
    }
    if (!canGoBack) {
        ImGui::EndDisabled();
    }

    ImGui::SameLine();

    // Fil d'ariane (Breadcrumb)
    std::string relPath = "";
    std::error_code ec;
    auto rel = fs::relative(currentDirectory_, baseDirectory_, ec);
    if (!ec) {
        relPath = rel.string();
        if (relPath == ".") relPath = "assets";
        else relPath = "assets / " + relPath;
    } else {
        relPath = currentDirectory_.filename().string();
    }

    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(ImVec4(0.4f, 0.75f, 1.0f, 1.0f), "Emplacement :");
    ImGui::SameLine();
    ImGui::Text("%s", relPath.c_str());

    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 130.0f);
    ImGui::SetNextItemWidth(120.0f);
    ImGui::SliderFloat("##ThumbSize", &thumbnailSize_, 48.0f, 128.0f, "Taille: %.0f");

    ImGui::Separator();

    // Grille d'éléments (fichiers et dossiers)
    float cellSize = thumbnailSize_ + padding_;
    float panelWidth = ImGui::GetContentRegionAvail().x;
    int columnCount = (int)(panelWidth / cellSize);
    if (columnCount < 1) columnCount = 1;

    ImGui::Columns(columnCount, 0, false);

    if (fs::exists(currentDirectory_) && fs::is_directory(currentDirectory_)) {
        std::vector<fs::directory_entry> entries;
        for (const auto& entry : fs::directory_iterator(currentDirectory_)) {
            entries.push_back(entry);
        }

        // Trier : dossiers d'abord, puis fichiers par ordre alphabétique
        std::sort(entries.begin(), entries.end(), [](const fs::directory_entry& a, const fs::directory_entry& b) {
            if (a.is_directory() != b.is_directory()) {
                return a.is_directory();
            }
            return a.path().filename().string() < b.path().filename().string();
        });

        for (const auto& entry : entries) {
            const auto& path = entry.path();
            std::string filenameStr = path.filename().string();

            ImGui::PushID(filenameStr.c_str());

            bool isDir = entry.is_directory();
            std::string ext = path.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(::tolower(c)); });

            // Couleur personnalisée selon le type d'asset (comme dans Hazel Engine)
            ImVec4 btnColor = isDir ? ImVec4(0.22f, 0.30f, 0.40f, 0.85f) :
                              (ext == ".json") ? ImVec4(0.20f, 0.38f, 0.25f, 0.85f) :
                              (ext == ".dxf")  ? ImVec4(0.40f, 0.25f, 0.20f, 0.85f) :
                              (ext == ".cs")   ? ImVec4(0.35f, 0.20f, 0.40f, 0.85f) :
                                                 ImVec4(0.20f, 0.205f, 0.21f, 0.85f);

            ImGui::PushStyleColor(ImGuiCol_Button, btnColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(btnColor.x + 0.1f, btnColor.y + 0.1f, btnColor.z + 0.1f, 1.0f));

            // Libellé de bouton
            char iconLabel[32];
            if (isDir) {
                snprintf(iconLabel, sizeof(iconLabel), "[DIR]");
            } else if (ext == ".json") {
                snprintf(iconLabel, sizeof(iconLabel), "[JSON]");
            } else if (ext == ".dxf") {
                snprintf(iconLabel, sizeof(iconLabel), "[CAD]");
            } else if (ext == ".cs") {
                snprintf(iconLabel, sizeof(iconLabel), "[C#]");
            } else {
                snprintf(iconLabel, sizeof(iconLabel), "[FILE]");
            }

            if (ImGui::Button(iconLabel, ImVec2(thumbnailSize_, thumbnailSize_ * 0.7f))) {
                if (isDir) {
                    currentDirectory_ /= path.filename();
                }
            }

            // Double clic pour ouvrir / charger
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                if (isDir) {
                    currentDirectory_ /= path.filename();
                } else if (ext == ".json") {
                    pendingLoadFile_ = path.string();
                } else if (ext == ".dxf") {
                    pendingDxfFile_ = path.string();
                } else if (ext == ".cs") {
                    pendingScriptFile_ = path.string();
                }
            }

            ImGui::PopStyleColor(2);

            // Libellé tronqué avec infobulle complète
            std::string displayLabel = filenameStr;
            if (displayLabel.length() > 14) {
                displayLabel = displayLabel.substr(0, 11) + "...";
            }
            ImGui::TextWrapped("%s", displayLabel.c_str());
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s\nDouble-clic pour charger", filenameStr.c_str());
            }

            ImGui::NextColumn();
            ImGui::PopID();
        }
    }

    ImGui::Columns(1);
    ImGui::End();
}
