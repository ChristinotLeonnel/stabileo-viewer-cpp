#include "ui/HazelUI.h"
#include <imgui_internal.h>
#include <filesystem>
#include <cstdio>
#include <string>

namespace Hazel::UI {

    void InitFonts(ImGuiIO& io) {
        ImFontConfig config;
        config.OversampleH = 3;
        config.OversampleV = 2;
        config.PixelSnapH = true;

        const char* regularCandidates[] = {
            "assets/fonts/Regular.ttf",
            "../assets/fonts/Regular.ttf",
            "../../assets/fonts/Regular.ttf",
            "C:\\Windows\\Fonts\\segoeui.ttf",
            "C:\\Windows\\Fonts\\arial.ttf"
        };

        const char* boldCandidates[] = {
            "assets/fonts/Bold.ttf",
            "../assets/fonts/Bold.ttf",
            "../../assets/fonts/Bold.ttf",
            "C:\\Windows\\Fonts\\segoeuib.ttf",
            "C:\\Windows\\Fonts\\arialbd.ttf"
        };

        std::string regPath;
        for (const char* p : regularCandidates) {
            if (std::filesystem::exists(p)) {
                regPath = p;
                break;
            }
        }

        std::string bldPath;
        for (const char* p : boldCandidates) {
            if (std::filesystem::exists(p)) {
                bldPath = p;
                break;
            }
        }

        if (!regPath.empty()) {
            // Police principale Segoe UI / Open Sans agrandie (20.5px) pour un confort de lecture optimal
            io.FontDefault = io.Fonts->AddFontFromFileTTF(regPath.c_str(), 20.5f, &config, io.Fonts->GetGlyphRangesDefault());

            // Police grasse assortie (20.5px) pour titres, boutons X, Y, Z
            if (!bldPath.empty()) {
                io.Fonts->AddFontFromFileTTF(bldPath.c_str(), 20.5f, &config, io.Fonts->GetGlyphRangesDefault());
            }
        } else {
            io.Fonts->AddFontDefault();
        }
    }

    void SetDarkThemeColors() {
        ImGuiStyle& style = ImGui::GetStyle();

        // Lissage anti-aliasing haute qualité
        style.AntiAliasedLines       = true;
        style.AntiAliasedLinesUseTex = true;
        style.AntiAliasedFill        = true;

        // Géométrie et arrondis de style Hazelnut Editor
        style.WindowRounding    = 7.0f;
        style.ChildRounding     = 5.0f;
        style.FrameRounding     = 4.0f;
        style.PopupRounding     = 5.0f;
        style.ScrollbarRounding = 6.0f;
        style.GrabRounding      = 4.0f;
        style.TabRounding       = 5.0f;
        style.DockingSeparatorSize = 2.0f;

        style.WindowPadding     = ImVec2(8.0f, 8.0f);
        style.FramePadding      = ImVec2(6.0f, 4.0f);
        style.ItemSpacing       = ImVec2(6.0f, 5.0f);
        style.ItemInnerSpacing  = ImVec2(4.0f, 4.0f);
        style.ScrollbarSize     = 12.0f;
        style.GrabMinSize       = 10.0f;
        style.WindowBorderSize  = 1.0f;
        style.FrameBorderSize   = 0.0f;
        style.PopupBorderSize   = 1.0f;

        // Palette sombre officielle de Hazel Engine (The Cherno)
        ImVec4* colors = style.Colors;

        // Base text
        colors[ImGuiCol_Text]                  = ImVec4(0.90f, 0.90f, 0.92f, 1.00f);
        colors[ImGuiCol_TextDisabled]          = ImVec4(0.48f, 0.50f, 0.55f, 1.00f);

        // Window background : #18181A (sombre, très élégant et neutre)
        colors[ImGuiCol_WindowBg]              = ImVec4(0.10f, 0.105f, 0.11f, 1.00f);
        colors[ImGuiCol_ChildBg]               = ImVec4(0.12f, 0.125f, 0.13f, 1.00f);
        colors[ImGuiCol_PopupBg]               = ImVec4(0.11f, 0.115f, 0.12f, 0.98f);

        // Borders
        colors[ImGuiCol_Border]                = ImVec4(0.20f, 0.205f, 0.21f, 0.85f);
        colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

        // Headers (Arborescences, Sélection, CollapsingHeaders)
        colors[ImGuiCol_Header]                = ImVec4(0.20f, 0.205f, 0.21f, 1.00f);
        colors[ImGuiCol_HeaderHovered]         = ImVec4(0.30f, 0.305f, 0.31f, 1.00f);
        colors[ImGuiCol_HeaderActive]          = ImVec4(0.15f, 0.1505f, 0.151f, 1.00f);

        // Buttons
        colors[ImGuiCol_Button]                = ImVec4(0.20f, 0.205f, 0.21f, 1.00f);
        colors[ImGuiCol_ButtonHovered]         = ImVec4(0.30f, 0.305f, 0.31f, 1.00f);
        colors[ImGuiCol_ButtonActive]          = ImVec4(0.15f, 0.1505f, 0.151f, 1.00f);

        // Frame BG (Inputs, Sliders, Checkboxes)
        colors[ImGuiCol_FrameBg]               = ImVec4(0.20f, 0.205f, 0.21f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.30f, 0.305f, 0.31f, 1.00f);
        colors[ImGuiCol_FrameBgActive]         = ImVec4(0.15f, 0.1505f, 0.151f, 1.00f);

        // Tabs
        colors[ImGuiCol_Tab]                   = ImVec4(0.15f, 0.1505f, 0.151f, 1.00f);
        colors[ImGuiCol_TabHovered]            = ImVec4(0.38f, 0.3805f, 0.381f, 1.00f);
        colors[ImGuiCol_TabActive]             = ImVec4(0.28f, 0.2805f, 0.281f, 1.00f);
        colors[ImGuiCol_TabUnfocused]          = ImVec4(0.15f, 0.1505f, 0.151f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.20f, 0.205f, 0.21f, 1.00f);

        // Title Bar
        colors[ImGuiCol_TitleBg]               = ImVec4(0.15f, 0.1505f, 0.151f, 1.00f);
        colors[ImGuiCol_TitleBgActive]         = ImVec4(0.15f, 0.1505f, 0.151f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.15f, 0.1505f, 0.151f, 1.00f);
        colors[ImGuiCol_MenuBarBg]             = ImVec4(0.13f, 0.135f, 0.14f, 1.00f);

        // Scrollbars & Sliders
        colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.10f, 0.105f, 0.11f, 0.60f);
        colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.28f, 0.285f, 0.29f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.36f, 0.365f, 0.37f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.44f, 0.445f, 0.45f, 1.00f);
        colors[ImGuiCol_CheckMark]             = ImVec4(0.35f, 0.65f, 0.95f, 1.00f);
        colors[ImGuiCol_SliderGrab]            = ImVec4(0.35f, 0.65f, 0.95f, 1.00f);
        colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.45f, 0.75f, 1.00f, 1.00f);

        // Separator & Resize grips
        colors[ImGuiCol_Separator]             = ImVec4(0.22f, 0.225f, 0.23f, 1.00f);
        colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.35f, 0.65f, 0.95f, 0.78f);
        colors[ImGuiCol_SeparatorActive]       = ImVec4(0.35f, 0.65f, 0.95f, 1.00f);
        colors[ImGuiCol_ResizeGrip]            = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
        colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);

        // Docking
        colors[ImGuiCol_DockingPreview]        = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
        colors[ImGuiCol_DockingEmptyBg]        = ImVec4(0.08f, 0.085f, 0.09f, 1.00f);

        // Tables
        colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.18f, 0.185f, 0.19f, 1.00f);
        colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.25f, 0.255f, 0.26f, 1.00f);
        colors[ImGuiCol_TableBorderLight]      = ImVec4(0.19f, 0.195f, 0.20f, 1.00f);
    }

    bool DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue, float columnWidth, float speed) {
        bool modified = false;
        ImGuiIO& io = ImGui::GetIO();
        auto boldFont = (io.Fonts->Fonts.Size > 1) ? io.Fonts->Fonts[1] : io.Fonts->Fonts[0];

        ImGui::PushID(label.c_str());

        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text("%s", label.c_str());
        ImGui::NextColumn();

        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 2.0f, 2.0f });

        float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
        ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

        // --- Bouton X (Rouge de The Cherno) ---
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.25f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
        ImGui::PushFont(boldFont);
        if (ImGui::Button("X", buttonSize)) {
            values.x = resetValue;
            modified = true;
        }
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        if (ImGui::DragFloat("##X", &values.x, speed, 0.0f, 0.0f, "%.3f")) {
            modified = true;
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();

        // --- Bouton Y (Vert de The Cherno) ---
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
        ImGui::PushFont(boldFont);
        if (ImGui::Button("Y", buttonSize)) {
            values.y = resetValue;
            modified = true;
        }
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        if (ImGui::DragFloat("##Y", &values.y, speed, 0.0f, 0.0f, "%.3f")) {
            modified = true;
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();

        // --- Bouton Z (Bleu de The Cherno) ---
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
        ImGui::PushFont(boldFont);
        if (ImGui::Button("Z", buttonSize)) {
            values.z = resetValue;
            modified = true;
        }
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        if (ImGui::DragFloat("##Z", &values.z, speed, 0.0f, 0.0f, "%.3f")) {
            modified = true;
        }
        ImGui::PopItemWidth();

        ImGui::PopStyleVar();
        ImGui::Columns(1);
        ImGui::PopID();

        return modified;
    }

    bool DrawFloatControl(const std::string& label, float& value, float resetValue, float columnWidth, float speed, float min, float max, const char* format) {
        bool modified = false;
        ImGui::PushID(label.c_str());

        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text("%s", label.c_str());
        ImGui::NextColumn();

        float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
        ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 2.0f, 2.0f });

        // Bouton Reset R
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4{ 0.3f, 0.3f, 0.35f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.4f, 0.4f, 0.45f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4{ 0.25f, 0.25f, 0.30f, 1.0f });
        if (ImGui::Button("R", buttonSize)) {
            value = resetValue;
            modified = true;
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::DragFloat("##Val", &value, speed, min, max, format)) {
            modified = true;
        }

        ImGui::PopStyleVar();
        ImGui::Columns(1);
        ImGui::PopID();

        return modified;
    }

    bool BeginComponent(const std::string& name, bool defaultOpen) {
        ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_Framed |
                                           ImGuiTreeNodeFlags_SpanAvailWidth |
                                           ImGuiTreeNodeFlags_AllowOverlap |
                                           ImGuiTreeNodeFlags_FramePadding;
        if (defaultOpen) {
            treeNodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;
        }

        ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4.0f, 4.0f });
        float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
        ImGui::Separator();

        bool open = ImGui::TreeNodeEx((void*)typeid(std::string).hash_code(), treeNodeFlags, "%s", name.c_str());
        ImGui::PopStyleVar();

        ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
        if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight })) {
            ImGui::OpenPopup("ComponentSettings");
        }

        if (ImGui::BeginPopup("ComponentSettings")) {
            if (ImGui::MenuItem("Réinitialiser les paramètres")) {
                // Action contextuelle
            }
            if (ImGui::MenuItem("Copier les valeurs")) {
                // Action copier
            }
            ImGui::EndPopup();
        }

        return open;
    }

    void EndComponent() {
        ImGui::TreePop();
    }

} // namespace Hazel::UI
